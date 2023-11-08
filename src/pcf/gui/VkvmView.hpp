/**
 * @file VkvmView.hpp
 * @author Daniel Starke
 * @date 2019-10-07
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_VKVMVIEW_HPP__
#define __PCF_GUI_VKVMVIEW_HPP__

#include <mutex>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/fl_draw.H>
#include <FL/gl.h>
#include <pcf/gui/Utility.hpp>
#include <pcf/video/Capture.hpp>


namespace pcf {
namespace gui {


/**
 * Video capture device viewer.
 */
class VkvmView : public Fl_Gl_Window, public pcf::video::CaptureCallback {
public:
	enum Rotation {
		ROT_UP = 0,
		ROT_RIGHT = 1,
		ROT_DOWN = 2,
		ROT_LEFT = 3,
		ROT_DEFAULT = ROT_UP
	};
private:
	enum Flag {
		MIRROR_RIGHT = USERFLAG1,
		MIRROR_UP = USERFLAG2
	};
	pcf::video::CaptureDevice * capDev;
	GLvoid * lastImage;
	size_t lastImageSize;
	GLsizei lastWidth;
	GLsizei lastHeight;
	GLenum lastFormat;
	GLenum lastType;
	mutable std::mutex captureMutex; /**< locked while drawing or capture update */
	Fl_Callback * capResizeCb; /**< called if the capture image size changed */
	void * capResizeCbArg;
	Fl_Callback * clickCb; /**< called if the user clicked on the widget */
	void * clickCbArg;
	Rotation curRotation;
public:
	explicit VkvmView(const int X, const int Y, const int W, const int H);

	virtual ~VkvmView();

	pcf::video::CaptureDevice * captureDevice() const { return this->capDev; }
	bool captureDevice(pcf::video::CaptureDevice * dev);

	inline size_t captureWidth() const { return size_t(((int(this->curRotation) & 1) == 0) ? this->lastWidth : this->lastHeight); }
	inline size_t captureHeight() const { return size_t(((int(this->curRotation) & 1) == 0) ? this->lastHeight : this->lastWidth); }

	inline Rotation rotation() const { return this->curRotation; }
	inline void rotation(const Rotation val) {
		if (val != this->curRotation) {
			const bool wasResized = ((int(val) ^ int(this->curRotation)) & 1) != 0;
			this->curRotation = val;
			this->redraw();
			if ( wasResized ) {
				this->doCaptureResizeCallback();
			}
		}
	}
	inline bool mirrorRight() const { return flags() & static_cast<unsigned int>(MIRROR_RIGHT); }
	inline void mirrorRight(const bool val) { updateStyle(static_cast<unsigned int>(MIRROR_RIGHT), val); }
	inline bool mirrorUp() const { return flags() & static_cast<unsigned int>(MIRROR_UP); }
	inline void mirrorUp(const bool val) { updateStyle(static_cast<unsigned int>(MIRROR_UP), val); }

	inline Fl_Callback * captureResizeCallback() const { return this->capResizeCb; }
	inline void * captureResizeCallbackArg() const { return this->capResizeCbArg; }
	inline void captureResizeCallback(Fl_Callback * cb, void * arg = NULL) {
		this->capResizeCb = cb;
		this->capResizeCbArg = arg;
	}
	inline void doCaptureResizeCallback() {
		if (this->capResizeCb != NULL) {
			(*(this->capResizeCb))(this->as_window(), this->capResizeCbArg);
		}
	}

	inline Fl_Callback * clickCallback() const { return this->clickCb; }
	inline void * clickCallbackArg() const { return this->clickCbArg; }
	inline void clickCallback(Fl_Callback * cb, void * arg = NULL) {
		this->clickCb = cb;
		this->clickCbArg = arg;
	}
	inline void doClickCallback() {
		if (this->clickCb != NULL) {
			(*(this->clickCb))(this->as_window(), this->clickCbArg);
		}
	}
protected:
	virtual int handle(int e);
	virtual void draw();

	inline void updateStyle(const unsigned int f, const bool on) {
		unsigned int oldFlags = flags();
		on ? set_flag(f) : clear_flag(f);
		if (oldFlags != flags()) redraw();
	}

	virtual void onCapture(const pcf::color::Rgb24 * img, const size_t width, const size_t height);
	virtual void onCapture(const pcf::color::Bgr24 * img, const size_t width, const size_t height);
private:
	void updateImage(const GLenum format, const GLenum datType, const GLvoid * img, const size_t width, const size_t height, const size_t byteSize);
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_VKVMVIEW_HPP__ */
