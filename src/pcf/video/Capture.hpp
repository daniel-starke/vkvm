/**
 * @file Capture.hpp
 * @author Daniel Starke
 * @date 2019-10-01
 * @version 2023-10-03
 */
#ifndef __PCF_VIDEO_CAPTURE_HPP__
#define __PCF_VIDEO_CAPTURE_HPP__

#include <cstddef>
#include <cstdint>
#include <vector>
#include <Fl/x.H> /* needed for the typedef of Window */
#include <pcf/color/Utility.hpp>
#include <pcf/Cloneable.hpp>


namespace pcf {
namespace video {


/* Forward declaration. */
class CaptureDevice;


/**
 * Callback interface to be implemented to receive captured images.
 */
class CaptureCallback {
public:
	/** Destructor. */
	virtual ~CaptureCallback() {}

	/**
	 * Called with the captured image as RGB24 array in the given dimensions.
	 *
	 * @param[in] image - image data
	 * @param[in] width - image width
	 * @param[in] height - image height
	 * @remarks This may be called from a different thread.
	 */
	virtual void onCapture(const pcf::color::Rgb24 * image, const size_t width, const size_t height) = 0;

	/**
	 * Called with the captured image as BGR24 array in the given dimensions.
	 *
	 * @param[in] image - image data
	 * @param[in] width - image width
	 * @param[in] height - image height
	 * @remarks This may be called from a different thread.
	 */
	virtual void onCapture(const pcf::color::Bgr24 * image, const size_t width, const size_t height) = 0;
};


/**
 * Callback interface to be implemented to receive capture device change notifications.
 */
class CaptureDeviceChangeCallback {
public:
	/** Destructor. */
	virtual ~CaptureDeviceChangeCallback() {}

	/**
	 * Called if a new capture device was detected.
	 *
	 * @param[in] device - added device path
	 * @remarks This may be called from a different thread.
	 */
	virtual void onCaptureDeviceArrival(const char * device) = 0;

	/**
	 * Called if a capture device was removed.
	 *
	 * @param[in] device - remove device path
	 * @remarks This may be called from a different thread.
	 */
	virtual void onCaptureDeviceRemoval(const char * device) = 0;
};


/**
 * Interface of a single capture device.
 */
class CaptureDevice : public CloneableInterface<CaptureDevice> {
protected:
	CaptureCallback * callback; /**< callback function */
	volatile bool running; /**< currently capturing? */
public:
	/** Possible result codes for setConfiguration(). */
	enum ReturnCode {
		RC_SUCCESS, /**< The operation completed successfully. */
		RC_ERROR_INV_ARG, /**< An invalid argument was given. */
		RC_ERROR_INV_SYNTAX /**< The given value uses an invalid syntax. */
	};
public:
	/** Constructor. */
	explicit inline CaptureDevice():
		callback(NULL),
		running(false)
	{}

	/** Destructor. */
	virtual ~CaptureDevice() {}

	/**
	 * Returns the unique path of the capture device. The returned pointer shell not be freed.
	 *
	 * @return capture device path
	 * @remarks The function is not re-entrant safe.
	 */
	virtual const char * getPath() = 0;

	/**
	 * Returns the human readable name of the capture device. The returned pointer shell not be freed.
	 *
	 * @return capture device name
	 * @remarks The function is not re-entrant safe.
	 */
	virtual const char * getName() = 0;

	/**
	 * Opens a window to configure the capture device.
	 *
	 * @param[in,out] wnd - use this parent window
	 */
	virtual void configure(Window wnd) = 0;

	/**
	 * Returns the current configuration of the capture device. The returned pointer needs to be freed.
	 *
	 * @return capture device configuration
	 */
	virtual char * getConfiguration() = 0;

	/**
	 * Changes the current configuration of the capture device to the provided one.
	 *
	 * @param[in] val - new configuration to use
	 * @param[out] errPos - optionally sets the parsing position on error
	 * @return success state
	 */
	virtual ReturnCode setConfiguration(const char * val, const char ** errPos) = 0;

	/**
	 * Starts the video capture procedure with this device.
	 * A modal window is opened with capture source settings if none have
	 * been set previously via setConfiguration().
	 *
	 * @param[in,out] wnd - use this parent window
	 * @param[in] cb - send capture images to this callback
	 * @return true on success, else false
	 */
	virtual bool start(Window wnd, CaptureCallback & cb) = 0;

	/**
	 * Stops the video capture procedure.
	 *
	 * @return true on success, else false
	 */
	virtual bool stop() = 0;

	/**
	 * Returns the video capture procedure state.
	 *
	 * @return true if currently capturing, else false
	 */
	inline bool isRunning() const { return this->running; }
};


/**
 * Video capture device list. See CaptureDeviceProvider on how to receive and free this list.
 */
typedef std::vector<CaptureDevice *> CaptureDeviceList;


/**
 * Interface to handle video capture device lists.
 */
class CaptureDeviceProvider {
public:
	/** Destructor. */
	virtual ~CaptureDeviceProvider() {}

	/**
	 * Returns a list of available capture devices.
	 *
	 * @return capture device list
	 */
	virtual CaptureDeviceList getDeviceList() = 0;

	/**
	 * Frees the previously returned list of capture devices.
	 *
	 * @param[in,out] list - list of capture devices to free
	 */
	virtual void freeDeviceList(CaptureDeviceList & list) {
		for (CaptureDevice * dev : list) {
			delete dev;
		}
		list.clear();
	}
};


/**
 * OS native capture device list provider. Use this to receive a list of capture devices available.
 */
class NativeVideoCaptureProvider : public CaptureDeviceProvider {
private:
	struct Pimple; /**< Implementation defined data structure. */
	Pimple * self; /**< Implementation defined data. */
public:
	/** Constructor. */
	explicit NativeVideoCaptureProvider();
	/** Destructor. */
	virtual ~NativeVideoCaptureProvider();

	/**
	 * Returns a list of available capture devices.
	 *
	 * @return capture device list
	 */
	virtual CaptureDeviceList getDeviceList();

	/**
	 * Add a callback which is called on device insertion or removal.
	 *
	 * @param[in] cb - callback instance
	 * @return true on success, else false
	 * @remarks The callback may be called from a different thread.
	 */
	static bool addNotificationCallback(CaptureDeviceChangeCallback & cb);

	/**
	 * Remove the given notification callback.
	 *
	 * @param[in] cb - callback instance
	 * @return true on success, else false
	 */
	static bool removeNotificationCallback(CaptureDeviceChangeCallback & cb);
};


} /* namespace video */
} /* namespace pcf */


#endif /* __PCF_VIDEO_CAPTURE_HPP__ */
