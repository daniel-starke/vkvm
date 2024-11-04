/**
 * @file CaptureVideo4Linux2.ipp
 * @author Daniel Starke
 * @date 2020-01-12
 * @version 2024-11-04
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <stdexcept>
#include <thread>
#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hor_Nice_Slider.H>
#include <FL/Fl_Value_Input.H>
#include <pcf/gui/HoverChoice.hpp>
#include <pcf/gui/ScrollableValueInput.hpp>
#include <pcf/gui/SvgButton.hpp>
#include <pcf/gui/SvgData.hpp>
#include <pcf/gui/SvgView.hpp>
#include <pcf/gui/Utility.hpp>
#include <pcf/video/Capture.hpp>
#include <pcf/ScopeExit.hpp>
#include <pcf/UtilityLinux.hpp>
extern "C" {
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>
#include <linux/videodev2.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
}


#ifndef PCF_MAX_SYS_PATH
#define PCF_MAX_SYS_PATH 1024
#endif


namespace pcf {
namespace video {
namespace {


/**
 * Helper class to manage capture device change notifications.
 */
class CaptureDeviceChangeNotifier {
private:
	int ed; /**< event descriptor to communicate with the internal thread */
	std::thread thread; /**< internal thread which checks for device changes */
	std::vector<CaptureDeviceChangeCallback *> callbacks; /**< list of callbacks which are called on device changes */
	std::mutex mutex; /**< mutex to make `addCallback()` and `removeCallback()` reentrant-safe */
	static CaptureDeviceChangeNotifier singleton; /**< global object instance */
public:
	/**
	 * Returns a single global `CaptureDeviceChangeNotifier` instance.
	 *
	 * @return `CaptureDeviceChangeNotifier` instance
	 */
	static inline CaptureDeviceChangeNotifier & getInstance() {
		return CaptureDeviceChangeNotifier::singleton;
	}

	/**
	 * Adds a new callback which shall receive device change notifications.
	 *
	 * @param[in] cb - callback to add
	 * @return true on success, else false if already added or on internal error
	 */
	inline bool addCallback(CaptureDeviceChangeCallback & cb) {
		std::lock_guard<std::mutex> guard(this->mutex);
		if ( ! this->thread.joinable() ) return false;
		for (CaptureDeviceChangeCallback * callback : this->callbacks) {
			if (callback == &cb) return false;
		}
		this->callbacks.push_back(&cb);
		return true;
	}

	/**
	 * Removes a given callback from the notification list.
	 *
	 * @param[in] cb - callback to remove
	 * @return true on success, else false if not present or on internal error
	 */
	inline bool removeCallback(CaptureDeviceChangeCallback & cb) {
		std::lock_guard<std::mutex> guard(this->mutex);
		if ( ! this->thread.joinable() ) return false;
		std::vector<CaptureDeviceChangeCallback *>::iterator it = this->callbacks.begin();
		for (; it != this->callbacks.end(); ++it) {
			if (*it == &cb) {
				this->callbacks.erase(it);
				return true;
			}
		}
		return false;
	}

	/**
	 * Destructor.
	 */
	inline ~CaptureDeviceChangeNotifier() {
		if ( this->thread.joinable() ) {
			if (this->ed >= 0) {
				uint64_t term = 1;
				while (write(this->ed, &term, sizeof(term)) != ssize_t(sizeof(term))) usleep(10000);
			}
			this->thread.join();
		}
		if (this->ed >= 0) close(this->ed);
	}
private:
	/**
	 * Constructor.
	 */
	explicit inline CaptureDeviceChangeNotifier():
		ed(eventfd(0, EFD_NONBLOCK))
	{
		if (this->ed < 0) throw std::runtime_error("CaptureDeviceChangeNotifier failed to create eventfd.");
		/* create thread to handle device changes */
		this->thread = std::thread(&CaptureDeviceChangeNotifier::threadProc, this);
	}

	/**
	 * Internal thread which checks for device changes and notifies the registered
	 * callback functions for each change.
	 */
	void threadProc() {
		NativeVideoCaptureProvider provider;
		const int fd = inotify_init1(IN_NONBLOCK);
		if (fd == -1) return;
		const auto closeFdOnReturn = makeScopeExit([=]() { close(fd); });
		const int wd = inotify_add_watch(fd, "/sys/class/video4linux", IN_CREATE | IN_DELETE | IN_DELETE_SELF);
		if (wd == -1) return;
		const auto closeWdOnReturn = makeScopeExit([=]() { close(wd); });
		struct timeval tout;
		fd_set fds;
		CaptureDeviceList oldList, newList = provider.getDeviceList();
		std::stable_sort(newList.begin(), newList.end());
		for ( ;; ) {
			FD_ZERO(&fds);
			FD_SET(this->ed, &fds);
			FD_SET(wd, &fds);
			tout.tv_sec = 0;
			tout.tv_usec = 500000;
			const int sRes = select(std::max(this->ed, wd) + 1, &fds, NULL, NULL, &tout);
			if (sRes < 0) {
				if (errno == EAGAIN || errno == EINTR) continue;
				break;
			}
			if (sRes > 0 && FD_ISSET(this->ed, &fds) != 0) break;
			if (sRes > 0 && FD_ISSET(wd, &fds) == 0) continue;
			/* timeout or directory watch -> update lists */
			oldList = std::move(newList);
			newList = provider.getDeviceList();
			std::stable_sort(newList.begin(), newList.end());
			/* check changes */
			{
				CaptureDeviceList::const_iterator itOld = oldList.begin();
				const CaptureDeviceList::const_iterator itOldEnd = oldList.end();
				CaptureDeviceList::const_iterator itNew = newList.begin();
				const CaptureDeviceList::const_iterator itNewEnd = newList.end();
				while (itOld != itOldEnd && itNew != itNewEnd) {
					if (strcmp((*itOld)->getPath(), (*itNew)->getPath()) < 0) {
						/* only in oldList */
						for (CaptureDeviceChangeCallback * callback : this->callbacks) {
							if (*itOld != NULL) {
								callback->onCaptureDeviceRemoval((*itOld)->getPath());
							}
						}
						++itOld;
					} else if (strcmp((*itNew)->getPath(), (*itOld)->getPath()) < 0) {
						/* only in newList */
						for (CaptureDeviceChangeCallback * callback : this->callbacks) {
							if (*itNew != NULL) {
								callback->onCaptureDeviceArrival((*itNew)->getPath());
							}
						}
						++itNew;
					} else {
						++itOld;
						++itNew;
					}
				}
				while (itOld != itOldEnd) {
					/* only in oldList */
					for (CaptureDeviceChangeCallback * callback : this->callbacks) {
						if (*itOld != NULL) {
							callback->onCaptureDeviceRemoval((*itOld)->getPath());
						}
					}
					itOld++;
				}
				while (itNew != itNewEnd) {
					/* only in newList */
					for (CaptureDeviceChangeCallback * callback : this->callbacks) {
						if (*itNew != NULL) {
							callback->onCaptureDeviceArrival((*itNew)->getPath());
						}
					}
					++itNew;
				}
			}
		}
	}
};


/** Global `CaptureDeviceChangeNotifier` object. */
CaptureDeviceChangeNotifier CaptureDeviceChangeNotifier::singleton;


/**
 * Capture source configuration window.
 */
class CaptureSourceConfigWindow : public Fl_Double_Window {
private:
	pcf::gui::HoverChoice * formatList;
	__u32 * formatTypes;
	size_t formatTypeSize;
	pcf::gui::HoverChoice * resolutionList;
	pcf::gui::ScrollableValueInput * resolutionWidth;
	pcf::gui::ScrollableValueInput * resolutionHeight;
	pcf::gui::HoverChoice * interleavingList;
	pcf::gui::SvgButton * ok;
	int videoFd;
	struct v4l2_format currentFmt;
	bool discreteResolution;
	bool res;
	bool done;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] L - window label (optional)
	 */
	explicit CaptureSourceConfigWindow(const char * L = NULL):
		Fl_Double_Window(pcf::gui::adjDpiH(240), pcf::gui::adjDpiV(160), L),
		formatTypes(NULL),
		formatTypeSize(0)
	{
		const int W = w();
		const int H = h();
		const int spaceH = pcf::gui::adjDpiH(10);
		const int spaceV = pcf::gui::adjDpiV(10);
		const int widgetV = pcf::gui::adjDpiV(26);
		const int labelH = pcf::gui::adjDpiH(36);
		const int xLabelH = pcf::gui::adjDpiH(15);
		const int valH = W - labelH - (2 * spaceH);
		int y1 = spaceV;

		pcf::gui::SvgView * formatLabel = new pcf::gui::SvgView(spaceH, y1, widgetV, widgetV, pcf::gui::formatSvg);
		formatLabel->tooltip("format");
		formatList = new pcf::gui::HoverChoice(spaceH + labelH, y1, valH, widgetV);
		formatList->tooltip("format");
		formatList->align(FL_ALIGN_LEFT);
		formatList->callback(PCF_GUI_CALLBACK(onFormatChange), this);
		y1 += widgetV + spaceV;

		pcf::gui::SvgView * resLabel = new pcf::gui::SvgView(spaceH, y1, widgetV, widgetV, pcf::gui::resolutionSvg);
		resLabel->tooltip("resolution");
		resLabel->colorView(true);
		resLabel->selection_color(FL_FOREGROUND_COLOR);
		resolutionList = new pcf::gui::HoverChoice(spaceH + labelH, y1, valH, widgetV);
		resolutionList->tooltip("resolution");
		resolutionList->align(FL_ALIGN_LEFT);

		resolutionWidth = new pcf::gui::ScrollableValueInput(spaceH + labelH, y1, (valH / 2) - (xLabelH / 2), widgetV);
		resolutionWidth->box(FL_THIN_DOWN_BOX);
		resolutionWidth->tooltip("resolution width");
		resolutionWidth->align(FL_ALIGN_LEFT);
		resolutionWidth->precision(0);

		resolutionHeight = new pcf::gui::ScrollableValueInput(W - (valH / 2) - spaceH + (xLabelH / 2), y1, (valH / 2) - (xLabelH / 2), widgetV, "x");
		resolutionHeight->box(FL_THIN_DOWN_BOX);
		resolutionHeight->tooltip("resolution height");
		resolutionHeight->align(FL_ALIGN_LEFT);
		resolutionHeight->precision(0);
		y1 += widgetV + spaceV;

		pcf::gui::SvgView * intLabel = new pcf::gui::SvgView(spaceH, y1, widgetV, widgetV, pcf::gui::interleavingSvg);
		intLabel->tooltip("interleaving");
		intLabel->colorView(true);
		intLabel->selection_color(FL_FOREGROUND_COLOR);
		interleavingList = new pcf::gui::HoverChoice(spaceH + labelH, y1, valH, widgetV);
		interleavingList->tooltip("interleaving");
		interleavingList->align(FL_ALIGN_LEFT);
		interleavingList->add("Any", 0, NULL);
		interleavingList->add("None", 0, NULL);
		interleavingList->add("Top", 0, NULL);
		interleavingList->add("Bottom", 0, NULL);
		interleavingList->add("Interlaced", 0, NULL);
		interleavingList->add("Top-Bottom", 0, NULL);
		interleavingList->add("Bottom-Top", 0, NULL);
		interleavingList->add("Alternate", 0, NULL);
		interleavingList->add("Interlaced Top-Bottom", 0, NULL);
		interleavingList->add("Interlaced Bottom-Top", 0, NULL);
		interleavingList->value(1);

		y1 = H - widgetV - spaceV;

		ok = new pcf::gui::SvgButton((W / 2) - widgetV - (spaceH / 2), y1, widgetV, widgetV, pcf::gui::okSvg);
		ok->colorButton(true);
		ok->labelcolor(FL_FOREGROUND_COLOR);
		ok->callback(PCF_GUI_CALLBACK(onOk), this);

		pcf::gui::SvgButton * cancel = new pcf::gui::SvgButton((W / 2) + (spaceH / 2), y1, widgetV, widgetV, pcf::gui::failSvg);
		cancel->colorButton(true);
		cancel->labelcolor(FL_FOREGROUND_COLOR);
		cancel->callback(PCF_GUI_CALLBACK(onCancel), this);

		set_modal();
		end();
	}

	/** Destructor. */
	virtual ~CaptureSourceConfigWindow() {
		if (this->formatTypes != NULL) free(this->formatTypes);
	}

	/**
	 * Returns the currently selected capture format.
	 *
	 * @return capture format
	 */
	inline __u32 getCaptureFormat() const {
		if (this->formatTypes == NULL || this->formatList->menu() == NULL) return this->currentFmt.fmt.pix.pixelformat;
		return this->formatTypes[this->formatList->value()];
	}

	/**
	 * Returns the currently selected capture width.
	 *
	 * @return capture width
	 */
	inline __u32 getCaptureWidth() const {
		if ( discreteResolution ) {
			unsigned long valW, valH;
			if (this->resolutionList->menu() == NULL || sscanf(resolutionList->text(), "%lux%lu", &valW, &valH) != 2) {
				return this->currentFmt.fmt.pix.width;
			}
			return static_cast<__u32>(valW);
		} else {
			return static_cast<__u32>(this->resolutionWidth->value());
		}
	}

	/**
	 * Returns the currently selected capture height.
	 *
	 * @return capture height
	 */
	inline __u32 getCaptureHeight() const {
		if ( discreteResolution ) {
			unsigned long valW, valH;
			if (this->resolutionList->menu() == NULL || sscanf(resolutionList->text(), "%lux%lu", &valW, &valH) != 2) {
				return this->currentFmt.fmt.pix.height;
			}
			return static_cast<__u32>(valH);
		} else {
			return static_cast<__u32>(this->resolutionHeight->value());
		}
	}

	/**
	 * Returns the currently selected capture interleaving field order.
	 *
	 * @return interleaving field order
	 */
	inline v4l2_field getCaptureFieldOrder() const {
		return static_cast<v4l2_field>(interleavingList->value());
	}

	/**
	 * Displays the capture source configuration window for the given capture device at the passed
	 * screen offset. The screen offset is mostly ignored due to the modal window flag.
	 *
	 * @param[in] fd - file descriptor of the capture device
	 * @param[in] X - window x-coordinate
	 * @param[in] Y - window y-coordinate
	 * @return true on ok, false if the user pressed cancel
	 */
	bool show(int fd, const int X, const int Y) {
		struct v4l2_fmtdesc fmtItem;
		bool gotFormat = false;
		int formatIndex = -1;
		position(X, Y);
		/* get current configuration */
		memset(&currentFmt, 0, sizeof(currentFmt));
		currentFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xEINTR(ioctl, fd, VIDIOC_G_FMT, &currentFmt) >= 0) {
			gotFormat = true;
		} else {
			fprintf(stderr, "Warning: ioctl failed for VIDIOC_G_FMT (%s)\n", strerror(errno));
		}
		/* fill lists with possible formats */
		memset(&fmtItem, 0, sizeof(fmtItem));
		fmtItem.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		formatList->clear();
		for (fmtItem.index = 0; xEINTR(ioctl, fd, VIDIOC_ENUM_FMT, &fmtItem) >= 0; fmtItem.index++) {
			if (formatTypeSize <= size_t(fmtItem.index)) extendFormatTypes();
			fmtItem.description[31] = 0;
			formatList->add(reinterpret_cast<const char *>(fmtItem.description), 0, NULL);
			formatTypes[fmtItem.index] = fmtItem.pixelformat;
			if (gotFormat && fmtItem.pixelformat == currentFmt.fmt.pix.pixelformat) formatIndex = int(fmtItem.index);
		}
		videoFd = fd;
		formatList->value((formatIndex >= 0) ? formatIndex : 0);
		resolutionList->redraw();
		onFormatChange(formatList); /* this fills the list with possible resolutions */
		ok->take_focus();
		/* the window is created from here on */
		Fl_Double_Window::show();
		done = false;
		res = true;
		while ( ! done ) Fl::wait();
		return res;
	}
protected:
	/**
	* Event handler.
	*
	* @param[in] e - event
	* @return 0 - if the event was not used or understood
	* @return 1 - if the event was used and can be deleted
	*/
	virtual int handle(int e) {
		switch (e) {
		case FL_HIDE:
			done = true;
			break;
		default:
			return Fl_Double_Window::handle(e);
			break;
		}
		return 1;
	}
private:
	PCF_GUI_BIND(CaptureSourceConfigWindow, onFormatChange, pcf::gui::HoverChoice);
	PCF_GUI_BIND(CaptureSourceConfigWindow, onOk, pcf::gui::SvgButton);
	PCF_GUI_BIND(CaptureSourceConfigWindow, onCancel, pcf::gui::SvgButton);

	/**
	 * Called on format list click to display a popup window with
	 * possible capture formats.
	 */
	void onFormatChange(pcf::gui::HoverChoice * /* list */) {
		if (this->formatList == NULL) return;
		if (this->formatList->menu() == NULL) {
			this->resolutionList->value(0);
			this->resolutionList->show();
			this->resolutionWidth->hide();
			this->resolutionHeight->hide();
			return;
		}
		char buf[64];
		struct v4l2_frmsizeenum resItem;
		/* fill list of possible resolutions for the selected pixel format */
		this->resolutionList->clear();
		memset(&resItem, 0, sizeof(resItem));
		resItem.pixel_format = this->formatTypes[this->formatList->value()];
		for (resItem.index = 0; xEINTR(ioctl, this->videoFd, VIDIOC_ENUM_FRAMESIZES, &resItem) >= 0; resItem.index++) {
			switch (resItem.type) {
			case V4L2_FRMSIZE_TYPE_DISCRETE:
				snprintf(buf, sizeof(buf), "%lux%lu", static_cast<unsigned long>(resItem.discrete.width), static_cast<unsigned long>(resItem.discrete.height));
				this->resolutionList->add(buf, 0, NULL);
				this->discreteResolution = true;
				break;
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			case V4L2_FRMSIZE_TYPE_STEPWISE:
				this->resolutionWidth->range(double(resItem.stepwise.min_width), double(resItem.stepwise.max_width));
				this->resolutionWidth->step(int(resItem.stepwise.step_width));
				this->resolutionWidth->value(int(resItem.stepwise.max_width));
				this->resolutionHeight->range(double(resItem.stepwise.min_height), double(resItem.stepwise.max_height));
				this->resolutionHeight->step(int(resItem.stepwise.step_height));
				this->resolutionWidth->value(int(resItem.stepwise.max_height));
				this->discreteResolution = false;
				break;
			default: break;
			}
		}
		/* select current resolution (if there is a matching one) */
		const unsigned long currentWidth = static_cast<unsigned long>(this->currentFmt.fmt.pix.width);
		const unsigned long currentHeight = static_cast<unsigned long>(this->currentFmt.fmt.pix.height);
		const int currentField = int(this->currentFmt.fmt.pix.field);
		if ( this->discreteResolution ) {
			this->resolutionWidth->hide();
			this->resolutionHeight->hide();
			this->resolutionList->show();
			snprintf(buf, sizeof(buf), "%lux%lu", currentWidth, currentHeight);
			const int index = resolutionList->find_index(buf);
			this->resolutionList->value((index >= 0) ? index : 0);
			this->formatList->redraw();
		} else {
			this->resolutionList->hide();
			this->resolutionWidth->show();
			this->resolutionHeight->show();
			this->resolutionWidth->value(double(currentWidth));
			this->resolutionHeight->value(double(currentHeight));
			this->resolutionWidth->redraw();
			this->resolutionHeight->redraw();
		}
		this->interleavingList->value(currentField);
		this->interleavingList->redraw();
	}

	/**
	 * Called on "Ok" button click. This closes the window and
	 * marks all values as valid.
	 */
	inline void onOk(pcf::gui::SvgButton * /* button */) {
		this->res = true;
		this->hide();
	}

	/**
	 * Called on "Cancel" button click. This closes the window
	 * and marks all values as invalid.
	 */
	inline void onCancel(pcf::gui::SvgButton * /* button */) {
		this->res = false;
		this->hide();
	}

	/**
	 * Extends the internal buffer for the format type array
	 * by 16.
	 */
	inline void extendFormatTypes() {
		const size_t newSize = this->formatTypeSize + 16;
		void * newTypes = realloc(this->formatTypes, sizeof(*(this->formatTypes)) * newSize);
		if (newTypes == NULL) throw std::bad_alloc();
		this->formatTypes = static_cast<__u32 *>(newTypes);
		this->formatTypeSize = newSize;
	}
};


/**
 * Capture configuration window.
 */
class CaptureConfigurationWindow : public Fl_Double_Window {
private:
	pcf::gui::SvgButton * ok;
	int videoFd;
	__s32 oldAutoBrightness;
	__s32 oldAutoWhiteBalance;
	__s32 oldAutoHue;
	__s32 oldAutoGain;
	__s32 oldBrightness;
	__s32 oldContrast;
	__s32 oldSaturation;
	__s32 oldGamma;
	__s32 oldHue;
	__s32 oldExposure;
	__s32 oldGain;
	__s32 oldFrequencyFilter;
	/**
	 * Holds a single capture configuration option.
	 * It is possible to link multiple entries in a
	 * singly linked list with this.
	 */
	struct Option {
		CaptureConfigurationWindow * self;
		Fl_Widget * optionValue;
		pcf::gui::SvgButton * optionAuto;
		const char * label;
		__u32 id;
		__u32 type;
		__u32 autoId;
		__s32 oldValue;
		__s32 oldAutoValue;
		__s32 defValue;
		__s32 defAutoValue;
		Option * next;
	};
	Option * options;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] dev - video capture device path
	 * @param[in] L - window label (optional)
	 */
	explicit CaptureConfigurationWindow(const char * dev, const char * L = NULL):
		Fl_Double_Window(pcf::gui::adjDpiH(320), 0, L),
		videoFd(-1),
		options(NULL)
	{
		const int W = w();
		const int spaceH = pcf::gui::adjDpiH(10);
		const int spaceV = pcf::gui::adjDpiV(10);
		const int widgetV = pcf::gui::adjDpiV(26);
		int y1 = spaceV;
		Option * lastOp = NULL;

		do {
			if (dev == NULL) break;
			videoFd = v4l2_open(dev, O_RDWR | O_NONBLOCK, 0);
			if (videoFd < 0) break;

			addOption(y1, lastOp, "brightness",             pcf::gui::brightnessSvg,   false, V4L2_CID_BRIGHTNESS, V4L2_CID_AUTOBRIGHTNESS);
			addOption(y1, lastOp, "contrast",               pcf::gui::contrastSvg,     false, V4L2_CID_CONTRAST);
			addOption(y1, lastOp, "saturation",             pcf::gui::saturationSvg,   false, V4L2_CID_HUE);
			addOption(y1, lastOp, "gamma",                  pcf::gui::gammaSvg,        true,  V4L2_CID_GAMMA);
			addOption(y1, lastOp, "white balance",          pcf::gui::whiteBalanceSvg, false, V4L2_CID_WHITE_BALANCE_TEMPERATURE, V4L2_CID_AUTO_WHITE_BALANCE);
			addOption(y1, lastOp, "hue",                    pcf::gui::hueSvg,          true,  V4L2_CID_HUE, V4L2_CID_HUE_AUTO);
			addOption(y1, lastOp, "exposure mode",          pcf::gui::exposureSvg,     true,  V4L2_CID_EXPOSURE_AUTO);
			addOption(y1, lastOp, "backlight compensation", pcf::gui::backlightSvg ,   true,  V4L2_CID_BACKLIGHT_COMPENSATION);
			addOption(y1, lastOp, "gain",                   pcf::gui::gainSvg,         false, V4L2_CID_GAIN, V4L2_CID_AUTOGAIN);
#ifdef V4L2_CID_SHARPNESS
			addOption(y1, lastOp, "sharpness",              pcf::gui::sharpnessSvg,    true,  V4L2_CID_SHARPNESS);
#endif /* V4L2_CID_SHARPNESS */
			addOption(y1, lastOp, "frequency filter",       pcf::gui::flickeringSvg,   true,  V4L2_CID_POWER_LINE_FREQUENCY);
		} while (false);

		/* ok */
		ok = new pcf::gui::SvgButton((W / 2) - (3 * widgetV / 2) - spaceH, y1, widgetV, widgetV, pcf::gui::okSvg);
		ok->colorButton(true);
		ok->labelcolor(FL_FOREGROUND_COLOR);
		ok->callback(PCF_GUI_CALLBACK(onOk), this);

		/* default */
		pcf::gui::SvgButton * def = new pcf::gui::SvgButton((W / 2) - (widgetV / 2), y1, widgetV, widgetV, pcf::gui::undoSvg);
		def->tooltip("set driver defaults");
		def->colorButton(true);
		def->labelcolor(FL_FOREGROUND_COLOR);
		def->callback(PCF_GUI_CALLBACK(onDef), this);

		/* cancel */
		pcf::gui::SvgButton * cancel = new pcf::gui::SvgButton((W / 2) + (widgetV / 2) + spaceH, y1, widgetV, widgetV, pcf::gui::failSvg);
		cancel->colorButton(true);
		cancel->labelcolor(FL_FOREGROUND_COLOR);
		cancel->callback(PCF_GUI_CALLBACK(onCancel), this);

		y1 += widgetV + spaceV;
		resizable(NULL);
		end();
		Fl_Double_Window::size(W, y1);
	}

	/**
	 * Destructor.
	 */
	virtual ~CaptureConfigurationWindow() {
		if (this->videoFd >= 0) close(this->videoFd);
		while (this->options != NULL) {
			Option * cur = this->options;
			this->options = cur->next;
			cur->optionValue->callback(static_cast<Fl_Callback *>(NULL), NULL); /* ensure that this resource is no longer used */
			delete cur;
		}
	}

	/**
	 * Displays the capture configuration window with the current device settings.
	 * This window is non-modal.
	 *
	 * @return true on ok, false if the user pressed cancel
	 */
	bool updateAndShow() {
		if (this->videoFd < 0) return false;
		if (this->visible() != 0) {
			/* bring to front */
			Fl_Double_Window::show();
			return false;
		}
		/* update GUI control from device values */
		__s32 val;
		for (Option * op = this->options; op != NULL; op = op->next) {
			if ( this->getControl(op->id, val) ) {
				this->setValue(*op, val);
				this->getValue(*op, val);
			}
			if (op->autoId != V4L2_CID_LASTP1 && op->optionAuto != NULL) {
				if ( this->getControl(op->autoId, val) ) {
					op->optionAuto->value((val != 0) ? 1 : 0);
					op->optionAuto->redraw();
					if (val != 0) {
						op->optionValue->deactivate();
					} else {
						op->optionValue->activate();
					}
				}
			}
		}
		this->ok->take_focus();
		/* the window is created from here on */
		Fl_Double_Window::show();
		return true;
	}
private:
	PCF_GUI_BIND(CaptureConfigurationWindow, onOk, pcf::gui::SvgButton);
	PCF_GUI_BIND(CaptureConfigurationWindow, onDef, pcf::gui::SvgButton);
	PCF_GUI_BIND(CaptureConfigurationWindow, onCancel, pcf::gui::SvgButton);

	/**
	 * Adds a video option control to the current window.
	 *
	 * @param[in,out] y1 - reference to the vertical window position
	 * @param[in,out] lastOp - option linked list to append
	 * @param[in] labelStr - option label
	 * @param[in] svg - option SVG string
	 * @param[in] colorize - colorize the SVG?
	 * @param[in] id - V4L2 specific option ID
	 * @param[in] autoId - V4L2 specific automatic value option ID if available (disable via V4L2_CID_LASTP1)
	 * @return true on success, else false
	 */
	bool addOption(int & y1, Option * & lastOp, const char * labelStr, const char * svg, const bool colorize, const __u32 id, const __u32 autoId = V4L2_CID_LASTP1) {
		struct v4l2_queryctrl ctrlQuery, autoQuery;
		const int ctrlTest = testControl(id, &ctrlQuery);
		const int autoTest = (autoId != V4L2_CID_LASTP1) ? testControl(autoId, &autoQuery) : -2;

		/* validity check */
		if (ctrlTest < 0 && autoTest < 0) return false; /* option is not available */
		switch (ctrlQuery.type) {
		case V4L2_CTRL_TYPE_INTEGER:
		case V4L2_CTRL_TYPE_BOOLEAN:
		case V4L2_CTRL_TYPE_MENU:
		case V4L2_CTRL_TYPE_INTEGER_MENU:
			break; /* supported */
		default:
			/* unsupported type of control */
			fprintf(stderr, "Warning: Control option \"%s\" uses an unexpected type of control.\n", labelStr);
			return false;
		}
		if (autoTest >= 0 && autoQuery.type != V4L2_CTRL_TYPE_BOOLEAN) {
			/* unsupported type of control */
			fprintf(stderr, "Warning: Control auto option \"%s\" uses an unexpected type of control.\n", labelStr);
			return false;
		}

		/* default constants */
		const int W = w();
		const int spaceH = pcf::gui::adjDpiH(10);
		const int spaceV = pcf::gui::adjDpiV(10);
		const int widgetV = pcf::gui::adjDpiV(26);
		const int labelH = pcf::gui::adjDpiH(36);
		const int valH = W - labelH - (2 * spaceH);

		/* new option element */
		Option * op = new Option{
			this, /* self */
			NULL, /* optionValue */
			NULL, /* optionAuto */
			labelStr, /* label */
			id, /* id */
			ctrlQuery.type, /* type */
			autoId, /* autoId */
			0, /* oldValue */
			0, /* oldAutoValue */
			ctrlQuery.default_value, /* defValue */
			autoQuery.default_value, /* defAutoValue */
			NULL /* next */
		};
		if (lastOp == NULL) {
			lastOp = op;
			this->options = op;
		} else {
			lastOp->next = op;
			lastOp = lastOp->next;
		}

		/* icon */
		if (svg != NULL) {
			pcf::gui::SvgView * optionLabel = new pcf::gui::SvgView(spaceH, y1, widgetV, widgetV, svg);
			optionLabel->tooltip(labelStr);
			if ( colorize ) {
				optionLabel->colorView(true);
				optionLabel->selection_color(FL_FOREGROUND_COLOR);
			}
		}

		/* value */
		const int oValH = (autoTest >= 0) ? valH - widgetV - spaceV : valH;
		if (ctrlTest >= 0 && this->getControl(id, op->oldValue)) {
			switch (ctrlQuery.type) {
			case V4L2_CTRL_TYPE_INTEGER:
				{
					Fl_Hor_Nice_Slider * optionValue = new Fl_Hor_Nice_Slider(spaceH + labelH, y1, oValH, widgetV);
					optionValue->range(double(ctrlQuery.minimum), double(ctrlQuery.maximum));
					optionValue->step(double(ctrlQuery.step));
					optionValue->value(double(op->oldValue));
					op->optionValue = static_cast<Fl_Widget *>(optionValue);
				}
				break;
			case V4L2_CTRL_TYPE_BOOLEAN:
				{
					Fl_Check_Button * optionValue = new Fl_Check_Button(spaceH + labelH, y1, oValH, widgetV, "ON");
					optionValue->value((op->oldValue != 0) ? 1 : 0);
					op->optionValue = static_cast<Fl_Widget *>(optionValue);
				}
				break;
			case V4L2_CTRL_TYPE_MENU:
				{
					pcf::gui::HoverChoice * optionValue = new pcf::gui::HoverChoice(spaceH + labelH, y1, oValH, widgetV);
					struct v4l2_querymenu menuQuery;
					memset(&menuQuery, 0, sizeof(menuQuery));
					menuQuery.id = id;
					int n = 0, sel = -1;
					for (menuQuery.index = __u32(ctrlQuery.minimum); menuQuery.index <= __u32(ctrlQuery.maximum); menuQuery.index++, n++) {
						if (xEINTR(ioctl, this->videoFd, VIDIOC_QUERYMENU, &menuQuery) < 0) continue;
						optionValue->add(reinterpret_cast<const char *>(menuQuery.name), 0, NULL, reinterpret_cast<void *>(menuQuery.index));
						if (menuQuery.index == __u32(op->oldValue)) sel = n;
					}
					if (sel >= 0) optionValue->value(sel);
					op->optionValue = static_cast<Fl_Widget *>(optionValue);
				}
				break;
			case V4L2_CTRL_TYPE_INTEGER_MENU:
				{
					struct v4l2_querymenu menuQuery;
					char buf[32];
					pcf::gui::HoverChoice * optionValue = new pcf::gui::HoverChoice(spaceH + labelH, y1, oValH, widgetV);
					memset(&menuQuery, 0, sizeof(menuQuery));
					menuQuery.id = id;
					int n = 0, sel = -1;
					for (menuQuery.index = __u32(ctrlQuery.minimum); menuQuery.index <= __u32(ctrlQuery.maximum); menuQuery.index++) {
						if (xEINTR(ioctl, this->videoFd, VIDIOC_QUERYMENU, &menuQuery) < 0) continue;
						snprintf(buf, sizeof(buf), "%lli", static_cast<long long>(menuQuery.value));
						optionValue->add(buf, 0, NULL, NULL);
						if (menuQuery.value == __s64(op->oldValue)) sel = n;
					}
					if (sel >= 0) optionValue->value(sel);
					op->optionValue = static_cast<Fl_Widget *>(optionValue);
				}
				break;
			default:
				/* unreachable */
				break;
			}
			op->optionValue->tooltip(labelStr);
			if (ctrlTest == 0) op->optionValue->deactivate();
		}

		/* auto button */
		if (autoTest >= 0 && this->getControl(autoId, op->oldAutoValue)) {
			op->optionAuto = new pcf::gui::SvgButton(oValH + (2 * spaceH) + labelH, y1, widgetV, widgetV, pcf::gui::autoSvg);
			op->optionAuto->type(FL_TOGGLE_BUTTON);
			op->optionAuto->tooltip("auto mode");
			op->optionAuto->hover(true);
			op->optionAuto->colorButton(true);
			op->optionAuto->labelcolor(FL_FOREGROUND_COLOR);
			op->optionAuto->selection_color(FL_FOREGROUND_COLOR);
			op->optionAuto->value((op->oldAutoValue != 0) ? 1 : 0);
			if (autoTest == 0) op->optionAuto->deactivate();
			if (op->oldAutoValue != 0) op->optionValue->deactivate();
		}

		/* install callbacks */
		if (op->optionValue != NULL) {
			op->optionValue->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
			op->optionValue->callback(&CaptureConfigurationWindow::onValueChange, op);
		}
		if (op->optionAuto != NULL) {
			op->optionAuto->callback(&CaptureConfigurationWindow::onAutoValueChange, op);
			op->optionAuto->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
		}

		y1 += widgetV + spaceV;
		return true;
	}

	/**
	 * Tests whether the given control ID is valid for this video device.
	 *
	 * @param[in] id - control ID
	 * @param[out] query - query result (optional)
	 * @return -2 on error, -1 if unsupported, 0 if supported but disabled, else 1
	 */
	inline int testControl(const __u32 id, struct v4l2_queryctrl * query = NULL) const {
		if (this->videoFd < 0) return -2;
		struct v4l2_queryctrl dummyQuery;
		if (query == NULL) query = &dummyQuery;
		memset(query, 0, sizeof(*query));
		query->id = id;
		int count = 0;
		while (xEINTR(ioctl, this->videoFd, VIDIOC_QUERYCTRL, query) < 0) {
			if (errno == EINVAL) return -1;
			if (errno != EBUSY || ++count > 3) return -2;
			usleep(10000);
		}
		if ((query->flags & V4L2_CTRL_FLAG_DISABLED) != 0) return 0;
		return 1;
	}

	/**
	 * Returns the current value for the given control ID.
	 *
	 * @param[in] id - control ID
	 * @param[out] val - output variable for the current value
	 * @return true on success, else false
	 */
	inline bool getControl(const __u32 id, __s32 & val) const {
		if (this->videoFd < 0) return false;
		struct v4l2_control control;
		memset(&control, 0, sizeof(control));
		control.id = id;
		int count = 0;
		while (xEINTR(ioctl, this->videoFd, VIDIOC_G_CTRL, &control) < 0) {
			if (errno != EBUSY || ++count > 3) return false;
			usleep(10000);
		}
		val = control.value;
		return true;
	}

	/**
	 * Sets a new value for the given control ID.
	 *
	 * @param[in] id - control ID
	 * @param[in] val - new value
	 * @return true on success, else false
	 */
	inline bool setControl(const __u32 id, const __s32 val) const {
		if (this->videoFd < 0) return false;
		struct v4l2_control control;
		memset(&control, 0, sizeof(control));
		control.id = id;
		control.value = val;
		int count = 0;
		while (xEINTR(ioctl, this->videoFd, VIDIOC_S_CTRL, &control) < 0) {
			if (errno != EBUSY || ++count > 3) return false;
			usleep(10000);
		}
		/* check whether the device really processed our value */
		__s32 newVal;
		if ( ! this->getControl(id, newVal) ) return false;
		return val == newVal;
	}

	/**
	 * Returns the current value of the given option.
	 *
	 * @param[in] op - option reference
	 * @param[out] val - output variable for the value
	 * @return true on success, else false
	 */
	inline bool getValue(const Option & op, __s32 & val) const {
		if (op.optionValue == NULL) return false;
		switch (op.type) {
		case V4L2_CTRL_TYPE_INTEGER:
			val = __s32(reinterpret_cast<Fl_Hor_Nice_Slider *>(op.optionValue)->value());
			break;
		case V4L2_CTRL_TYPE_BOOLEAN:
			val = __s32(reinterpret_cast<Fl_Check_Button *>(op.optionValue)->value());
			break;
		case V4L2_CTRL_TYPE_MENU:
			{
				pcf::gui::HoverChoice * optionValue = reinterpret_cast<pcf::gui::HoverChoice *>(op.optionValue);
				const int cur = optionValue->value();
				if (cur < 0 || cur >= optionValue->size()) return false;
				val = __s32(reinterpret_cast<std::uintptr_t>(optionValue->menu()[cur].user_data()));
			}
			break;
		case V4L2_CTRL_TYPE_INTEGER_MENU:
			{
				const char * cur = reinterpret_cast<pcf::gui::HoverChoice *>(op.optionValue)->text();
				char * endp;
				errno = 0;
				val = __s32(strtoll(cur, &endp, 10));
				if (*endp != 0 || cur == endp || errno != 0) return false;
			}
			break;
		default:
			/* unreachable */
			break;
		}
		return true;
	}

	/**
	 * Sets the value for the given option.
	 *
	 * @param[in] op - option reference
	 * @param[in] val - value to set
	 * @return true on success, else false
	 */
	inline bool setValue(const Option & op, const __s32 val) const {
		if (op.optionValue == NULL) return false;
		switch (op.type) {
		case V4L2_CTRL_TYPE_INTEGER:
			reinterpret_cast<Fl_Hor_Nice_Slider *>(op.optionValue)->value(int(val));
			break;
		case V4L2_CTRL_TYPE_BOOLEAN:
			reinterpret_cast<Fl_Check_Button *>(op.optionValue)->value((val != 0) ? 1 : 0);
			break;
		case V4L2_CTRL_TYPE_MENU:
			{
				pcf::gui::HoverChoice * optionValue = reinterpret_cast<pcf::gui::HoverChoice *>(op.optionValue);
				const int count = optionValue->size();
				for (int n = 0; n < count; n++) {
					if (__s32(reinterpret_cast<std::uintptr_t>(optionValue->menu()[n].user_data())) == val) {
						optionValue->value(n);
						break;
					}
				}
			}
			break;
		case V4L2_CTRL_TYPE_INTEGER_MENU:
			{
				char buf[32];
				snprintf(buf, sizeof(buf), "%lli", static_cast<long long>(val));
				pcf::gui::HoverChoice * optionValue = reinterpret_cast<pcf::gui::HoverChoice *>(op.optionValue);
				const int index = optionValue->find_index(buf);
				if (index < 0) return false;
				optionValue->value(index);
			}
			break;
		default:
			/* unreachable */
			break;
		}
		op.optionValue->redraw();
		return true;
	}

	/**
	 * GUI callback function called if the control value changed.
	 *
	 * @param[in,out] w - associated widget
	 * @param[in,out] userData - associated option reference
	 */
	static inline void onValueChange(Fl_Widget * /* w */, void * userData) {
		if (userData == NULL) return;
		Option * op = static_cast<Option *>(userData);
		if (op->self == NULL) return;
		/* get value from GUI control */
		__s32 val;
		if ( ! op->self->getValue(*op, val) ) return;
		/* set device control */
		if ( ! op->self->setControl(op->id, val) ) {
			/* revert GUI control on error */
			fprintf(stderr, "Error: Failed to set \"%s\" control value. %s\n", op->label, strerror(errno));
			if ( op->self->getControl(op->id, val) ) {
				op->self->setValue(*op, val);
			}
		}
	}

	/**
	 * GUI callback function called if the auto control value changed.
	 *
	 * @param[in,out] w - associated widget
	 * @param[in,out] userData - associated option reference
	 */
	static void onAutoValueChange(Fl_Widget * /* w */, void * userData) {
		if (userData == NULL) return;
		Option * op = static_cast<Option *>(userData);
		if (op->self == NULL) return;
		/* set device control from GUI control */
		if ( ! op->self->setControl(op->autoId, (op->optionAuto->value() != 0) ? 1 : 1) ) {
			/* revert GUI control on error */
			fprintf(stderr, "Error: Failed to set auto mode for \"%s\" control. %s\n", op->label, strerror(errno));
			__s32 val;
			if ( op->self->getControl(op->autoId, val) ) {
				op->optionAuto->value((val != 0) ? 1 : 0);
				op->optionAuto->redraw();
			}
			return;
		}
		/* update value control enable state in GUI */
		if (op->optionValue == NULL) return;
		if (op->optionAuto->value() != 0) {
			op->optionValue->deactivate();
		} else {
			op->optionValue->activate();
		}
		/* set current value from device */
		__s32 val;
		if ( op->self->getControl(op->id, val) ) {
			op->self->setValue(*op, val);
		}
	}

	/**
	 * GUI callback function called if the `OK` button was pressed.
	 *
	 * @param[in,out] button - associated button
	 */
	inline void onOk(pcf::gui::SvgButton * /* button */) {
		this->hide();
	}

	/**
	 * GUI callback function called if the `Default` button was pressed.
	 *
	 * @param[in,out] button - associated button
	 */
	void onDef(pcf::gui::SvgButton * /* button */) {
		for (Option * op = this->options; op != NULL; op = op->next) {
			if (op->autoId != V4L2_CID_LASTP1 && op->optionAuto != NULL) {
				this->setControl(op->autoId, op->defAutoValue);
				op->optionAuto->value((op->defAutoValue != 0) ? 1 : 0);
				op->optionAuto->redraw();
				if (op->defAutoValue == 0) {
					this->setControl(op->id, op->defValue);
					if (op->optionValue != NULL) {
						this->setValue(*op, op->defAutoValue);
						op->optionValue->activate();
					}
				} else {
					if (op->optionValue != NULL) {
						op->optionValue->deactivate();
					}
				}
			} else if (op->optionValue != NULL) {
				this->setControl(op->id, op->defValue);
				this->setValue(*op, op->defAutoValue);
			}
		}
	}

	/**
	 * GUI callback function called if the `Cancel` button was pressed.
	 *
	 * @param[in,out] button - associated button
	 */
	inline void onCancel(pcf::gui::SvgButton * /* button */) {
		for (Option * op = this->options; op != NULL; op = op->next) {
			if (op->autoId != V4L2_CID_LASTP1) {
				this->setControl(op->autoId, op->oldAutoValue);
				if (op->oldAutoValue == 0) {
					this->setControl(op->id, op->oldValue);
				}
			} else {
				this->setControl(op->id, op->oldValue);
			}
		}
		this->hide();
	}
};


class NativeCaptureDevice : public Cloneable<NativeCaptureDevice, CaptureDevice> {
public:
	typedef Cloneable<NativeCaptureDevice, CaptureDevice> Base;
private:
	/** Single capture buffer description. */
	struct CaptureBuffer {
		void * start; /**< memory mapped start pointer */
		size_t length; /**< data length */
	};
	char * devicePath; /**< capture device path */
	char * deviceName; /**< capture device name */
	CaptureConfigurationWindow * config; /**< capture device configuration window object */
	int fd; /**< capture device file descriptor */
	int ed; /**< event file descriptor to signal termination */
	struct CaptureBuffer bufferDesc[2]; /**< memory mapped regions for video capturing */
	__u32 bufferCount; /**< number of video buffers */
	std::thread thread; /**< video capture background thread */
	std::mutex mutex; /**< guards against multiple capture starts */
public:
	/**
	 * Constructor.
	 *
	 * @param[in] p - unique path of the video capture device
	 * @param[in] n - human readable video capture device name
	 */
	explicit inline NativeCaptureDevice(const char * p = NULL, const char * n = NULL):
		Base(),
		devicePath(NULL),
		deviceName(NULL),
		config(NULL),
		fd(-1),
		ed(-1),
		bufferCount(0)
	{
		this->initFrom(p, n);
		this->bufferDesc[0].start = MAP_FAILED;
		this->bufferDesc[1].start = MAP_FAILED;
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 */
	inline NativeCaptureDevice(const NativeCaptureDevice & o):
		Base(o),
		devicePath(NULL),
		deviceName(NULL),
		config(NULL),
		fd(-1),
		ed(-1),
		bufferCount(0)
	{
		this->initFrom(o.devicePath, o.deviceName);
		this->bufferDesc[0].start = MAP_FAILED;
		this->bufferDesc[1].start = MAP_FAILED;
	}

	/**
	 * Destructor.
	 */
	virtual ~NativeCaptureDevice() {
		this->stop();
		if (this->ed >= 0) close(this->ed);
		if (this->config != NULL) delete this->config;
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] o - object to assign
	 * @return this object
	 */
	inline NativeCaptureDevice & operator= (const NativeCaptureDevice & o) {
		if (this != &o) {
			this->stop();
			this->initFrom(o.devicePath, o.deviceName);
			if (this->config != NULL) {
				delete this->config;
				this->config = NULL;
			}
		}
		return *this;
	}

	/**
	 * Returns the unique path of the capture device. The returned pointer shell not be freed.
	 *
	 * @return capture device path
	 * @remarks The function is not re-entrant safe.
	 */
	virtual const char * getPath() {
		return this->devicePath;
	}

	/**
	 * Returns the human readable name of the capture device. The returned pointer shell not be freed.
	 *
	 * @return capture device name
	 * @remarks The function is not re-entrant safe.
	 */
	virtual const char * getName() {
		return this->deviceName;
	}

	/**
	 * Sets the human readable name of the serial port.
	 *
	 * @param[in] n - human readable serial port name
	 * @internal
	 */
	inline void setName(const char * n) {
		if (this->deviceName != NULL) free(this->deviceName);
		if (n == NULL) {
			this->deviceName = NULL;
			return;
		}
		const size_t len = strlen(n) + 1;
		this->deviceName = static_cast<char *>(malloc(len * sizeof(char)));
		if (this->deviceName != NULL) memcpy(this->deviceName, n, len * sizeof(char));
	}

	/**
	 * Opens a window to configure the capture device.
	 *
	 * @param[in,out] wnd - use this parent window
	 * @see https://www.kernel.org/doc/html/v4.14/media/uapi/v4l/control.html
	 * @see https://www.kernel.org/doc/html/v4.14/media/uapi/v4l/vidioc-queryctrl.html
	 */
	virtual void configure(Window /* wnd */) {
		if (this->config == NULL) {
			this->config = new CaptureConfigurationWindow(this->devicePath, this->deviceName);
			Fl_Window * w = Fl::first_window();
			if (w != NULL) {
				/* center to current window */
				this->config->position(w->x() + ((w->w() - this->config->w()) / 2), w->y() + ((w->h() - this->config->h()) / 2));
			}
		}
		this->config->updateAndShow();
	}

	/**
	 * Returns the current configuration of the capture device. The returned pointer needs to be freed.
	 *
	 * @return capture device configuration
	 */
	virtual char * getConfiguration() {
		// @todo
		return NULL;
	}

	/**
	 * Changes the current configuration of the capture device to the provided one.
	 *
	 * @param[in] val - new configuration to use
	 * @param[out] errPos - optionally sets the parsing position on error
	 * @return success state
	 */
	virtual ReturnCode setConfiguration(const char * /* val */, const char ** /* errPos */) {
		// @todo
		return RC_ERROR_INV_ARG;
	}

	/**
	 * Starts the video capture procedure with this device.
	 * A modal window is opened with capture source settings if none have
	 * been set previously via setConfiguration().
	 *
	 * @param[in,out] wnd - use this parent window
	 * @param[in] cb - send capture images to this callback
	 * @return true on success, else false
	 * @remarks `VIDIOC_SUBSCRIBE_EVENT` is not registered as buffer layout or format changes are
	 * guaranteed to happen only while streaming if turned off.
	 */
	virtual bool start(Window /* wnd */, CaptureCallback & cb) {
		struct v4l2_format captureFormat;
		struct v4l2_buffer buf;
		/* fresh start */
		if ( ! this->mutex.try_lock() ) return false;
		std::unique_lock<std::mutex> guard(this->mutex, std::adopt_lock);
		this->stopInternal();
		this->Base::callback = &cb;
		this->ed = eventfd(0, EFD_NONBLOCK);
		if (this->devicePath == NULL || this->ed < 0) return false;
		/* open capture device */
		this->fd = v4l2_open(this->devicePath, O_RDWR | O_NONBLOCK, 0);
		if (this->fd < 0) return false;
		/* set capture format */
		{
			CaptureSourceConfigWindow configWin("Capture Source Configuration");
			if ( ! configWin.show(this->fd, 0, 0) ) {
				this->stopInternal();
				return false;
			}
			/* set raw output format in V4L2 kernel driver */
			memset(&captureFormat, 0, sizeof(captureFormat));
			captureFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			captureFormat.fmt.pix.width = configWin.getCaptureWidth();
			captureFormat.fmt.pix.height = configWin.getCaptureHeight();
			captureFormat.fmt.pix.pixelformat = configWin.getCaptureFormat(); /* automatically converted to this by user space V4L2 lib */
			captureFormat.fmt.pix.field = configWin.getCaptureFieldOrder();
			if (xEINTR(ioctl, this->fd, VIDIOC_S_FMT, &captureFormat) < 0) {
				/* failed to set capture format */
				fprintf(stderr, "Warning: ioctl failed for VIDIOC_S_FMT with %lux%lu using %.4s (%s)\n", static_cast<unsigned long>(captureFormat.fmt.pix.width), static_cast<unsigned long>(captureFormat.fmt.pix.height), reinterpret_cast<const char *>(&(captureFormat.fmt.pix.pixelformat)), strerror(errno));
			}
			/* set output format in V4L2 user library (which does the conversion) */
			memset(&captureFormat, 0, sizeof(captureFormat));
			captureFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			captureFormat.fmt.pix.width = configWin.getCaptureWidth();
			captureFormat.fmt.pix.height = configWin.getCaptureHeight();
			captureFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24; /* automatically converted to this by user space V4L2 lib */
			captureFormat.fmt.pix.field = configWin.getCaptureFieldOrder();
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_S_FMT, &captureFormat) < 0 || captureFormat.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
				/* failed to set capture format */
				fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_S_FMT with %lux%lu using %.4s (%s)\n", static_cast<unsigned long>(captureFormat.fmt.pix.width), static_cast<unsigned long>(captureFormat.fmt.pix.height), reinterpret_cast<const char *>(&(captureFormat.fmt.pix.pixelformat)), strerror(errno));
				this->stopInternal();
				return false;
			}
		}
		/* initialize capture buffers */
		struct v4l2_requestbuffers req;
		memset(&req, 0, sizeof(req));
		req.count = 2; /* double buffering */
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;
		if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_REQBUFS, &req) < 0) {
			/* memory mapping is not supported */
			fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_REQBUFS with V4L2_MEMORY_MMAP (%s)\n", strerror(errno));
			this->stopInternal();
			return false;
		}
		this->bufferDesc[0].start = MAP_FAILED;
		this->bufferDesc[1].start = MAP_FAILED;
		this->bufferCount = req.count;
		for (__u32 n = 0; n < this->bufferCount; n++) {
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = n;
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_QUERYBUF, &buf) < 0) {
				/* failed to get buffer state */
				fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_QUERYBUF (%s)\n", strerror(errno));
				this->stopInternal();
				return false;
			}
			this->bufferDesc[n].length = buf.length;
			this->bufferDesc[n].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, buf.m.offset);
			if (this->bufferDesc[n].start == MAP_FAILED) {
				/* failed to obtain memory mapped user space region of the buffers */
				fprintf(stderr, "Error: v4l2_mmap failed (%s)\n", strerror(errno));
				this->stopInternal();
				return false;
			}
		}
		/* queue obtained buffers */
		for (__u32 n = 0; n < this->bufferCount; n++) {
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = n;
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_QBUF, &buf) < 0) {
				fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_QBUF with V4L2_MEMORY_MMAP (%s)\n", strerror(errno));
				/* failed to queue buffer */
				this->stopInternal();
				return false;
			}
		}
		/* start capture thread */
		this->thread = std::thread(&NativeCaptureDevice::threadProc, this);
		return true;
	}

	/**
	 * Stops the video capture procedure.
	 *
	 * @return true on success, else false
	 */
	virtual bool stop() {
		if ( ! this->mutex.try_lock() ) return false;
		const bool res = this->stopInternal();
		this->mutex.unlock();
		return res;
	}
private:
	/**
	 * Initializes the object from the given parameters.
	 *
	 * @param[in] p - unique path of the video capture device
	 * @param[in] n - human readable video capture device name
	 */
	inline void initFrom(const char * p, const char * n) {
		if (p != NULL) {
			const size_t len = strlen(p) + 1;
			this->devicePath = static_cast<char *>(malloc(len * sizeof(char)));
			if (this->devicePath != NULL) memcpy(this->devicePath, p, len * sizeof(char));
			this->setName(n);
		}
	}

	/**
	 * Background thread which retrieves the frames from the
	 * capture device and forwards those to the registered
	 * callback handlers.
	 *
	 * @remarks The thread will terminate on error or termination event.
	 */
	void threadProc() {
		struct v4l2_buffer buf;
		struct v4l2_format currentFmt;
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		struct timeval tout;
		fd_set fds;
		if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_STREAMON, &type) < 0) {
			fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_STREAMON (%s)\n", strerror(errno));
			return;
		}
		/* no buffer layout or format changes can be done from here on */
		const auto streamOffOnReturn = makeScopeExit([=]() mutable {
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_STREAMOFF, &type) < 0) {
				fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_STREAMOFF (%s)\n", strerror(errno));
			}
			/* buffer layout and format changes are possible again */
		});
		/* get current format */
		memset(&currentFmt, 0, sizeof(currentFmt));
		currentFmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_G_FMT, &currentFmt) < 0) {
			fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_G_FMT (%s)\n", strerror(errno));
			return;
		}
		switch (currentFmt.fmt.pix.pixelformat) {
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_BGR24:
			break;
		default:
			fprintf(stderr, "Error: invalid pixel format (%.4s)\n", reinterpret_cast<const char *>(&(currentFmt.fmt.pix.pixelformat)));
			return;
		}
		for ( ;; ) {
			FD_ZERO(&fds);
			FD_SET(this->ed, &fds);
			FD_SET(this->fd, &fds);
			tout.tv_sec = 2;
			tout.tv_usec = 0;
			errno = 0;
			const int sRes = select(std::max(this->ed, this->fd) + 1, &fds, NULL, NULL, &tout);
			if (sRes < 0) {
				if (errno == EAGAIN || errno == EINTR) continue;
				break;
			}
			if (sRes > 0 && FD_ISSET(this->ed, &fds) != 0) break;
			if (sRes == 0 || FD_ISSET(this->fd, &fds) == 0) continue; /* timeout */
			/* get a buffer with the received capture data */
			memset(&buf, 0, sizeof(buf));
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_DQBUF, &buf) < 0) {
				fprintf(stderr, "Warning: v4l2_ioctl failed for VIDIOC_DQBUF with V4L2_MEMORY_MMAP (%s)\n", strerror(errno));
				continue;
			}
			/* process data */
			switch (currentFmt.fmt.pix.pixelformat) {
			case V4L2_PIX_FMT_RGB24:
				this->callback->onCapture(
					reinterpret_cast<const pcf::color::Rgb24 *>(this->bufferDesc[buf.index].start),
					size_t(currentFmt.fmt.pix.width),
					size_t(currentFmt.fmt.pix.height)
				);
				break;
			case V4L2_PIX_FMT_BGR24:
				this->callback->onCapture(
					reinterpret_cast<const pcf::color::Bgr24 *>(this->bufferDesc[buf.index].start),
					size_t(currentFmt.fmt.pix.width),
					size_t(currentFmt.fmt.pix.height)
				);
				break;
			default:
				/* unreachable */
				break;
			}
			/* re-queue receive buffer */
			if (xEINTR(v4l2_ioctl, this->fd, VIDIOC_QBUF, &buf) < 0) {
				fprintf(stderr, "Error: v4l2_ioctl failed for VIDIOC_QBUF (%s)\n", strerror(errno));
			}
		}
	}

	/**
	 * Internal helper method to stop capturing.
	 * The caller needs to hold a lock to the mutex.
	 *
	 * @return true on success, else false
	 */
	bool stopInternal() {
		if ( this->thread.joinable() ) {
			if (this->ed >= 0) {
				uint64_t term = 1;
				while (write(this->ed, &term, sizeof(term)) != ssize_t(sizeof(term))) {
					usleep(10000);
				}
			}
			this->thread.join();
		}
		if (this->ed >= 0) {
			close(this->ed);
			this->ed = -1;
		}
		if (this->fd >= 0) {
			close(this->fd);
			this->fd = -1;
		}
		for (__u32 n = 0; n < this->bufferCount; n++) {
			if (this->bufferDesc[n].start != MAP_FAILED) {
				v4l2_munmap(this->bufferDesc[n].start, this->bufferDesc[n].length);
			}
		}
		return true;
	}
};


} /* anonymous namespace */


struct NativeVideoCaptureProvider::Pimple {
	/**
	 * Searched within all parent directories for a `idProduct`
	 * file and returns its path if found.
	 *
	 * @param[in,out] path - starting directory (modified for iteration and the result value)
	 * @param[in] baseLen - number of valid bytes in `path` for the base path (i.e. highest parent directory)
	 * @param[in] pathLen - number of valid bytes in `path`
	 * @return path to `idProduct` file or `NULL` if not found.
	 */
	static char * getParentWithIdProduct(char * path, const size_t baseLen, size_t pathLen) {
		if (path == NULL || baseLen <= 0) return NULL;
		struct stat st;
		while (baseLen < pathLen) {
			char * parentEnd = strrchr(path, '/');
			if (parentEnd == NULL) break;
			pathLen = size_t(pathLen - strlen(parentEnd));
			const ssize_t subPathLen = snprintf(path + pathLen + 1, PCF_MAX_SYS_PATH - pathLen - 1, "idProduct");
			if (subPathLen > 0 && subPathLen < ssize_t(PCF_MAX_SYS_PATH - pathLen - 1)) {
				if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
					parentEnd[1] = 0;
					return path;
				}
			}
			*parentEnd = 0;
		}
		path[baseLen] = 0;
		return NULL;
	}

	/**
	 * Collects a list of possible capture devices.
	 *
	 * @param[in,out] list - adds all capture devices found to this list
	 * @param[in,out] path - null-terminated path to search in (modified for iteration and the result value)
	 * @param[in] withNames - true to resolve device names, false for path only
	 * @return true if files were added, else false
	 */
	static bool getAvailableDevices(CaptureDeviceList & list, char * path, const bool withNames) {
		struct stat st;
		struct dirent ** dirList;
		int count = xEINTR(scandir, path, &dirList, nullptr, nullptr);
		if (count < 0) return false;
		const auto freeDirListOnReturn = makeScopeExit([=]() {
			for (int n = 0; n < count; n++) free(dirList[n]);
			free(dirList);
		});
		char * buffers = static_cast<char *>(malloc(3 * PCF_MAX_SYS_PATH * sizeof(char)));
		if (buffers == NULL) return false;
		const auto freeBuffersOnReturn = makeScopeExit([=]() { free(buffers); });
		char * buffer0 = buffers;
		char * buffer1 = buffers + PCF_MAX_SYS_PATH;
		char * buffer2 = buffers + (2 * PCF_MAX_SYS_PATH);
		const size_t origPathLen = strlen(path);
		const size_t remPathLen = size_t((origPathLen < PCF_MAX_SYS_PATH) ? PCF_MAX_SYS_PATH - origPathLen : 0);
		NativeCaptureDevice newDev;
		bool added = false;
		for (int n = 0; n < count; n++) {
			if (strcmp(dirList[n]->d_name, ".") == 0) continue;
			if (strcmp(dirList[n]->d_name, "..") == 0) continue;
			int pathSubLen = snprintf(path + origPathLen, remPathLen, "/%s/device/driver", dirList[n]->d_name);
			if (pathSubLen < 1 || pathSubLen >= int(remPathLen)) continue;
			if (xEINTR(lstat, path, &st) != 0 || ( ! S_ISLNK(st.st_mode) )) continue; /* no device driver assigned */
			const ssize_t linkLen = xEINTR(readlink, path, buffer0, PCF_MAX_SYS_PATH);
			if (linkLen <= 0 || linkLen >= PCF_MAX_SYS_PATH) continue; /* invalid or too long symbolic link */
			buffer0[linkLen] = 0;
			/* build final device path */
			const ssize_t devPathLen = snprintf(buffer1, PCF_MAX_SYS_PATH, "/dev/%s", dirList[n]->d_name);
			if (devPathLen <= 0 || devPathLen >= PCF_MAX_SYS_PATH) continue;
			buffer1[devPathLen] = 0;
			/* test if capture device */
			{
				const int fd = xEINTR(open, buffer1, O_RDONLY);
				if (fd < 0) continue;
				const auto closeFdAtEndOfScope = makeScopeExit([=]() { close(fd); });
				struct v4l2_capability caps;
				if (xEINTR(ioctl, fd, VIDIOC_QUERYCAP, &caps) < 0) continue;
				if ((caps.device_caps & V4L2_CAP_VIDEO_CAPTURE) == 0) continue;
				newDev = std::move(NativeCaptureDevice(buffer1));
			}
			/* get friendly name from name file */
			const ssize_t devNamePathLen = snprintf(buffer2, PCF_MAX_SYS_PATH, "%.*s/%s/name", int(origPathLen), path, dirList[n]->d_name);
			if (devNamePathLen <= 0 || devNamePathLen >= PCF_MAX_SYS_PATH) continue;
			buffer2[devNamePathLen] = 0;
			const int nFd = xEINTR(open, buffer2, O_RDONLY);
			if (nFd < 0) break; /* failed to open name file */
			const ssize_t bytesRead = xEINTR(read, nFd, buffer2, PCF_MAX_SYS_PATH - 1);
			close(nFd);
			char * friendlyName = NULL;
			if (bytesRead > 0) {
				buffer2[bytesRead] = 0;
				memcpy(buffer0, buffer2, bytesRead + 1);
				friendlyName = buffer0;
			} else {
				/* build friendly name from driver name */
				friendlyName = strrchr(buffer0, '/');
				if (friendlyName == NULL) continue;
				friendlyName++;
			}
			/* get name for USB devices: readlink ./device, get parent directory, read product file */
			while ( withNames ) {
				/* resolve path of /sys/class/video4linux/xxx */
				path[origPathLen + pathSubLen - 14] = 0;
				memcpy(buffer1, path, origPathLen);
				buffer1[origPathLen] = '/';
				buffer1[origPathLen + 1] = 0;
				/* obtain full path by appending symbolic link to /sys/class/video4linux/ */
				const ssize_t devLinkPathLen = xEINTR(readlink, path, buffer1 + origPathLen + 1, size_t(PCF_MAX_SYS_PATH - origPathLen - 1));
				if (devLinkPathLen <= 0 || devLinkPathLen >= ssize_t(PCF_MAX_SYS_PATH - origPathLen - 1)) break;
				/* assume this device is owned by an USB device and get its product name */
				size_t basePathLen = size_t(origPathLen + devLinkPathLen + 1);
				buffer1[basePathLen] = 0;
				const size_t extOrigPathLen = origPathLen + ((strncmp("../../devices/", buffer1 + origPathLen + 1, 14) == 0) ? 15 : 1);
				char * idProductPath = getParentWithIdProduct(buffer1, extOrigPathLen, basePathLen);
				if (idProductPath == NULL) break; /* device has no parent with a idProduct file -> is not a USB device */
				basePathLen = strlen(buffer1);
				const ssize_t productPathLen = snprintf(buffer1 + basePathLen, PCF_MAX_SYS_PATH - basePathLen, "product");
				if (productPathLen < 0 || productPathLen >= ssize_t(PCF_MAX_SYS_PATH - basePathLen)) break; /* path too long */
				buffer1[basePathLen + productPathLen] = 0;
				const int fd = xEINTR(open, buffer1, O_RDONLY);
				if (fd < 0) break; /* failed to open USB product file */
				const ssize_t bytesRead2 = xEINTR(read, fd, buffer2, PCF_MAX_SYS_PATH - 1);
				close(fd);
				if (bytesRead2 <= 0) break; /* empty USB product name */
				buffer2[bytesRead2] = 0;
				for (ssize_t k = 0; k < bytesRead2; k++) {
					if (buffer2[k] < ' ') {
						buffer2[k] = 0;
						break;
					}
				}
				/* successfully derived product name of the owning USB device */
				friendlyName = buffer2;
				break;
			};
			/* add device to list if we got to this point */
			const char * devIdx = strrchr(newDev.getPath(), 'o');
			if (devIdx == NULL) {
				newDev.setName(friendlyName);
			} else {
				/* add with device index */
				snprintf(buffer1, PCF_MAX_SYS_PATH, "%s: %s", devIdx + 1, friendlyName);
				newDev.setName(buffer1);
			}
			list.push_back(new NativeCaptureDevice(newDev));
			added = true;
		}
		return added;
	}
};


NativeVideoCaptureProvider::NativeVideoCaptureProvider():
	self(NULL)
{}


NativeVideoCaptureProvider::~NativeVideoCaptureProvider() {
	if (self != NULL) delete self;
}


CaptureDeviceList NativeVideoCaptureProvider::getDeviceList() {
	CaptureDeviceList result;

	/* get list of available capture devices */
	char path[PCF_MAX_SYS_PATH] = "/sys/class/video4linux";
	NativeVideoCaptureProvider::Pimple::getAvailableDevices(result, path, true);

	/* return device list */
	return result;
}


bool NativeVideoCaptureProvider::addNotificationCallback(CaptureDeviceChangeCallback & cb) {
	return CaptureDeviceChangeNotifier::getInstance().addCallback(cb);
}


bool NativeVideoCaptureProvider::removeNotificationCallback(CaptureDeviceChangeCallback & cb) {
	return CaptureDeviceChangeNotifier::getInstance().removeCallback(cb);
}


} /* namespace video */
} /* namespace pcf */
