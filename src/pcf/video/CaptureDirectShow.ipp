/**
 * @file CaptureDirectShow.ipp
 * @author Daniel Starke
 * @date 2019-10-03
 * @version 2023-10-26
 * @todo rework with new MF API:
 *  - https://www.dreamincode.net/forums/topic/347938-a-new-webcam-api-tutorial-in-c-for-windows/
 *  - https://www.codeproject.com/Articles/776058/Capturing-Live-video-from-Web-camera-on-Windows-an
 */
#include <stdexcept>
#include <libpcf/cvutf8.h>
#include <libpcf/tchar.h>
#include <pcf/video/Capture.hpp>
#include <pcf/ScopeExit.hpp>
#include <windows.h>
#include <dshow.h>
#include <qedit.h>
#include <dbt.h>
#include <ks.h>


#ifndef MAXLONGLONG
#define MAXLONGLONG 0x7FFFFFFFFFFFFFFF
#endif


/** Capture stream control value forwarded if started. */
#define CTRL_STARTED 1
/** Capture stream control value forwarded if stopped. */
#define CTRL_STOPPED 2


#ifdef __MINGW32__
/* MinGW does not understand COM interfaces. */
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif


namespace pcf {
namespace video {
namespace {


/** Class ID for the `SampleGrabber`. */
static const CLSID CLSID_SampleGrabber = {0xC1F400A0, 0x3F08, 0x11D3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};
/** Class ID for the `NullRenderer`. */
static const CLSID CLSID_NullRenderer  = {0xC1F400A4, 0x3F08, 0x11D3, {0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37}};


/**
 * Frees an allocated media type structure.
 *
 * @param[in,out] mt - media type structure to free
 */
inline void FreeMediaType(AM_MEDIA_TYPE & mt) {
	if (mt.cbFormat != 0) {
		CoTaskMemFree(static_cast<PVOID>(mt.pbFormat));
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if (mt.pUnk != NULL) {
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}


/**
 * Frees an allocated media type instance.
 *
 * @param[in,out] pmt - media type instance to free
 */
inline void DeleteMediaType(AM_MEDIA_TYPE * pmt) {
	if (pmt != NULL) {
		FreeMediaType(*pmt);
		CoTaskMemFree(pmt);
	}
}


/**
 * Checks whether the given video filter pin is connected or not.
 *
 * @param[in] pPin - filter pin to check
 * @param[out] pResult - variable receiving the result
 * @return `S_OK` on success, else an `HRESULT` error code
 */
inline HRESULT IsPinConnected(IPin * pPin, BOOL * pResult) {
	if (pPin == NULL || pResult == NULL) return E_POINTER;
	IPin * pTmp = NULL;
	HRESULT res = pPin->ConnectedTo(&pTmp);
	if ( SUCCEEDED(res) ) {
		*pResult = TRUE;
	} else if (res == VFW_E_NOT_CONNECTED) {
		*pResult = FALSE;
		res = S_OK;
	}
	if (pTmp != NULL) pTmp->Release();
	return res;
}


/**
 * Implementation of the `ISampleGrabberCB` interface to process frames recorded
 * by the `SampleGrabber` instance.
 */
class CallbackHandler : public ISampleGrabberCB {
private:
	CaptureCallback * callback; /**< associated VKVM capture callback handle */
	pcf::color::ColorFormat::Type colorFormat; /**< color format of the captured frames */
	size_t width; /**< width of the captured frames */
	size_t height; /**< height of the captured frames */
	CRITICAL_SECTION mutex; /**< guard the internal structures from concurrent capture events */
	ULONG refCount; /**< reference counter for automatic deletion */
public:
	/**
	 * Constructor.
	 *
	 * @param[in,out] cb - VKVM capture callback handle
	 */
	explicit inline CallbackHandler(CaptureCallback & cb):
		callback(&cb),
		colorFormat(pcf::color::ColorFormat::UNKNOWN),
		width(0),
		height(0),
		refCount(1)
	{
		InitializeCriticalSection(&(this->mutex));
	}

	/** Destructor. */
	inline virtual ~CallbackHandler() {
		DeleteCriticalSection(&(this->mutex));
	}

	/**
	 * Set a new VKVM capture callback handle.
	 *
	 * @param[in] cb - new VKVM capture callback handle
	 */
	inline void setCallback(CaptureCallback & cb) {
		this->callback = &cb;
	}

	/**
	 * Called if the capture format has been changed.
	 *
	 * @param[in] cf - color format
	 * @param[in] w - width
	 * @param[in] h - height
	 */
	inline void setOutputFormat(const pcf::color::ColorFormat::Type cf, const size_t w, const size_t h) {
		this->colorFormat = cf;
		this->width = w;
		this->height = h;
	}

	/**
	 * Implementation of the `ISampleGrabberCB::SampleCB()` method.
	 *
	 * @param[in] time - starting time of the sample in seconds
	 * @param[in] sample - pointer to the `IMediaSample` interface of the sample
	 * @return `S_OK` on success, else an `HRESULT` error code
	 * @see https://learn.microsoft.com/en-us/windows/win32/directshow/isamplegrabbercb-samplecb
	 */
	virtual HRESULT __stdcall SampleCB(double /* time */, IMediaSample * sample) {
		HRESULT res;
		AM_MEDIA_TYPE * mt;
		BYTE * buffer;
		EnterCriticalSection(&(this->mutex));
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });

		res = sample->GetPointer(&buffer);
		if (res != S_OK) return S_OK;

		/* update output resolution and format on change */
		res = sample->GetMediaType(&mt);
		if (res < 0) return S_OK;
		if (mt != NULL) {
			const auto deleteMtOnReturn = makeScopeExit([&]() { DeleteMediaType(mt); });
			pcf::color::ColorFormat::Type newCf = pcf::color::ColorFormat::UNKNOWN;
			if (mt->subtype == MEDIASUBTYPE_RGB24) {
				newCf = pcf::color::ColorFormat::BGR_24;
			} else {
				return VFW_E_INVALIDMEDIATYPE;
			}
			if (mt->formattype != FORMAT_VideoInfo || mt->cbFormat < sizeof(VIDEOINFOHEADER) || mt->pbFormat == NULL) {
				return VFW_E_INVALIDMEDIATYPE;
			}
			const VIDEOINFOHEADER * videoInfoHeader = reinterpret_cast<const VIDEOINFOHEADER *>(mt->pbFormat);
			this->setOutputFormat(newCf, size_t(videoInfoHeader->bmiHeader.biWidth), size_t(videoInfoHeader->bmiHeader.biHeight));
		}

		/* pass data to user callback */
		switch (this->colorFormat) {
		case pcf::color::ColorFormat::BGR_24:
			this->callback->onCapture(reinterpret_cast<const pcf::color::Bgr24 *>(buffer), this->width, this->height);
			break;
		default:
			return VFW_E_INVALIDMEDIATYPE;
		}

		return S_OK;
	}

	/**
	 * Implementation of the `ISampleGrabberCB::BufferCB()` method.
	 *
	 * @param[in] time - starting time of the sample in seconds
	 * @param[in] buffer - pointer to a buffer that contains the sample data
	 * @param[in] len - buffer length in bytes
	 * @return `S_OK` on success, else an `HRESULT` error code
	 * @see https://learn.microsoft.com/en-us/windows/win32/directshow/isamplegrabbercb-buffercb
	 */
	virtual HRESULT __stdcall BufferCB(double /* time */, BYTE * /* buffer */, long /* len */) {
		return E_NOTIMPL;
	}

	/**
	 * Implementation of the `IUnknown::AddRef()` method.
	 *
	 * @return new reference count
	 * @see https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-addref
	 */
	virtual ULONG __stdcall AddRef() {
		return InterlockedIncrement(&(this->refCount));
	}

	/**
	 * Implementation of the `IUnknown::QueryInterface()` method.
	 *
	 * @param[in] riid - reference to the interface identifier (IID) of the interface being queried for
	 * @param[in] ppvObj - address of a pointer to an interface with the specified IID
	 * @return `S_OK` if the interface is supported, else `E_NOINTERFACE` (`E_POINTER` if `ppvObj` is `NULL`)
	 * @see https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-queryinterface(refiid_void)
	 */
	virtual HRESULT __stdcall QueryInterface(REFIID riid, LPVOID * ppvObj) {
		if (ppvObj == NULL) return E_POINTER;
		if (riid == IID_IUnknown || riid == IID_ISampleGrabberCB) {
			*ppvObj = static_cast<LPVOID>(this);
			this->AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	/**
	 * Implementation of the `IUnknown::Release()` method.
	 *
	 * @return new reference count
	 * @see https://learn.microsoft.com/en-us/windows/win32/api/unknwn/nf-unknwn-iunknown-release
	 */
	virtual ULONG __stdcall Release() {
		const ULONG count = InterlockedDecrement(&(this->refCount));
	    if (count == 0) delete this;
	    return count;
	}
};


/**
 * Helper class to manage capture device change notifications.
 */
class CaptureDeviceChangeNotifier {
private:
	HWND hWnd; /**< window handle which receives events */
	HDEVNOTIFY hNotify; /**< device change notification handle */
	HANDLE hThread; /**< internal thread which creates and handles events associated to the window handle */
	std::vector<CaptureDeviceChangeCallback *> callbacks; /**< list of callbacks which are called on device changes */
	CRITICAL_SECTION mutex; /**< mutex to make `addCallback()` and `removeCallback()` reentrant-safe */
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
		EnterCriticalSection(&(this->mutex));
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });
		if ( ! this->isRunning() ) return false;
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
		EnterCriticalSection(&(this->mutex));
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });
		if ( ! this->isRunning() ) return false;
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
		if (this->hWnd != NULL) {
			PostMessage(this->hWnd, WM_CLOSE, 0, 0);
		}
		if (this->hThread != NULL) {
			WaitForSingleObject(this->hThread, INFINITE);
			CloseHandle(this->hThread);
		}
		DeleteCriticalSection(&(this->mutex));
	}
private:
	/**
	 * Constructor.
	 */
	inline explicit CaptureDeviceChangeNotifier():
		hWnd(NULL),
		hNotify(NULL),
		hThread(NULL)
	{
		/* create critical section */
		InitializeCriticalSection(&(this->mutex));
		/* create thread to handle window messages */
		this->hThread = CreateThread(NULL, 0, CaptureDeviceChangeNotifier::threadProc, this, 0, NULL);
	}

	/**
	 * Checks whether the internal thread is still running.
	 *
	 * @return true if the internal thread is running, else false
	 */
	inline bool isRunning() const {
		if (this->hThread == NULL) return false;
		const DWORD result = WaitForSingleObject(this->hThread, 0);
		if (result == WAIT_OBJECT_0) {
			DWORD exitCode = 0;
			if (GetExitCodeThread(this->hThread, &exitCode) == 0 || exitCode == STILL_ACTIVE) return false;
		}
		return true;
	}

	/**
	 * Window callback handler which is called for each event associated to hWnd.
	 *
	 * @param[in] hWnd - handle of the window which received the event
	 * @param[in] uMsg - event message ID
	 * @param[in] wParam - wParam of the event
	 * @param[in] lParam - lParam of the event
	 * @return window callback result code
	 */
	static LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		CaptureDeviceChangeNotifier * self = reinterpret_cast<CaptureDeviceChangeNotifier *>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
		switch (uMsg) {
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		case WM_DESTROY:
			if (self->hNotify != NULL) {
				UnregisterDeviceNotification(self->hNotify);
				self->hNotify = NULL;
			}
			PostQuitMessage(0);
			break;
		case WM_DEVICECHANGE:
			if (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE) {
				DEV_BROADCAST_HDR * pHdr = reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (pHdr != NULL && pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
					DEV_BROADCAST_DEVICEINTERFACE * pDi = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE *>(pHdr);
#if defined(UNICODE) || defined(_UNICODE)
					char * devPath = cvutf8_fromUtf16(pDi->dbcc_name);
#else
					char * devPath = pDi->dbcc_name;
#endif
					if (devPath != NULL) {
						EnterCriticalSection(&(self->mutex));
						if (wParam == DBT_DEVICEARRIVAL) {
							for (CaptureDeviceChangeCallback * callback : self->callbacks) {
								callback->onCaptureDeviceArrival(devPath);
							}
						} else {
							for (CaptureDeviceChangeCallback * callback : self->callbacks) {
								callback->onCaptureDeviceRemoval(devPath);
							}
						}
						LeaveCriticalSection(&(self->mutex));
#if defined(UNICODE) || defined(_UNICODE)
						free(devPath);
#endif
					}
				}
			}
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;
		}
		return 0;
	}

	/**
	 * Internal thread that creates the window which receives device change notifications
	 * and handles those. The registered callback functions are called for each device
	 * change.
	 */
	static DWORD WINAPI threadProc(LPVOID lpParameter) {
		static const char * className = "CaptureDirectShow::CaptureDeviceChangeNotifier";
		static bool wndClassRegistered = false;
		if (lpParameter == NULL) return EXIT_FAILURE;
		CaptureDeviceChangeNotifier * self = reinterpret_cast<CaptureDeviceChangeNotifier *>(lpParameter);
		HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
		/* window creation code needs to be within the same thread as the message loop */
		/* register window class */
		if ( ! wndClassRegistered ) {
			WNDCLASSEXA wc;
			wc.cbSize        = sizeof(WNDCLASSEXA);
			wc.style         = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc   = CaptureDeviceChangeNotifier::windowProc;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = hInstance;
			wc.hIcon         = NULL;
			wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
			wc.lpszMenuName  = NULL;
			wc.lpszClassName = className;
			wc.hIconSm       = NULL;
			if (RegisterClassExA(&wc) != 0) {
				wndClassRegistered = true;
			}
		}
		if ( ! wndClassRegistered ) {
			throw std::runtime_error("Registering window class CaptureDirectShow::CaptureDeviceChangeNotifier failed.");
		}
		/* create window to receive device notification messages */
		self->hWnd = CreateWindowExA(
			WS_EX_CLIENTEDGE, className, className, WS_DISABLED | WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, 1, 1,
			NULL, NULL, hInstance, NULL
		);
		if (self->hWnd == NULL) {
			throw std::runtime_error("Creation of CaptureDirectShow::CaptureDeviceChangeNotifier window failed.");
		}
		/* bind this instance to the window */
		SetWindowLongPtrA(self->hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
		/* register device notification */
		DEV_BROADCAST_DEVICEINTERFACE notificationFilter;
		memset(&notificationFilter, 0, sizeof(DEV_BROADCAST_DEVICEINTERFACE));
		notificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
		notificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
		notificationFilter.dbcc_classguid  = KSCATEGORY_CAPTURE;
		self->hNotify = RegisterDeviceNotification(self->hWnd, &notificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
		if (self->hNotify == NULL) {
			throw std::runtime_error("Registration of CaptureDirectShow::CaptureDeviceChangeNotifier device notifier failed.");
		}
		/* window message loop */
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return EXIT_SUCCESS;
	}
};


/** Global `CaptureDeviceChangeNotifier` object. */
CaptureDeviceChangeNotifier CaptureDeviceChangeNotifier::singleton;


class NativeCaptureDevice : public Cloneable<NativeCaptureDevice, CaptureDevice> {
public:
	typedef Cloneable<NativeCaptureDevice, CaptureDevice> Base;
private:
	CRITICAL_SECTION mutex; /**< guard against parallel access */
	IMoniker * moniker; /**< video capture device */
	IFilterGraph2 * graph; /**< randering graph */
	ICaptureGraphBuilder2 * capture; /**< capture graph builder */
	IMediaControl * control; /**< capture control */
	IBaseFilter * sourceFilter; /**< video source filter */
	IBaseFilter * nullRenderer; /**< null output renderer used to terminate the rendering pipeline */
	IBaseFilter * sampleGrabberFilter; /**< image grabber filter to forward the video capture frames */
	ISampleGrabber * sampleGrabber; /**< actual image grabber */
	CallbackHandler * callbackHandler; /**< callback receiving the grabbed image frames */
	bool attachedSourceFilter; /**< source filter was attached to the capture device? */
	bool attachedSampleGrabber; /**< image grabber was attached to the rendering graph? */
	bool setMediaType; /**< output media type has been configured? */
	bool attachedRenderer; /**< capture graph has been connected to source filters? */
	bool configuredSourceOutput; /**< output format video capture has been configured? */
	bool attachedControl; /**< control interface have been attached to the capture graph? */
	bool setOutputFormat; /**< output resolution and format has been set? */
	char * devicePath; /**< video capture device path */
	char * deviceName; /**< video capture device name */
public:
	/**
	 * Constructor.
	 *
	 * @param[in,out] aMoniker - video capture device descriptor
	 */
	explicit inline NativeCaptureDevice(IMoniker * aMoniker = NULL):
		Base(),
		moniker(aMoniker),
		graph(NULL),
		capture(NULL),
		control(NULL),
		sourceFilter(NULL),
		nullRenderer(NULL),
		sampleGrabberFilter(NULL),
		sampleGrabber(NULL),
		callbackHandler(NULL),
		attachedSourceFilter(false),
		attachedSampleGrabber(false),
		setMediaType(false),
		attachedRenderer(false),
		configuredSourceOutput(false),
		attachedControl(false),
		setOutputFormat(false),
		devicePath(NULL),
		deviceName(NULL)
	{
		if (this->moniker != NULL) {
			this->moniker->AddRef();
			/* ensure that the variables are set in case of device removal */
			this->getName();
			this->getPath();
		}
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 */
	inline NativeCaptureDevice(const NativeCaptureDevice & o):
		Base(o),
		moniker(o.moniker),
		graph(o.graph),
		capture(o.capture),
		control(o.control),
		sourceFilter(o.sourceFilter),
		nullRenderer(o.nullRenderer),
		sampleGrabberFilter(o.sampleGrabberFilter),
		sampleGrabber(o.sampleGrabber),
		callbackHandler(o.callbackHandler),
		attachedSourceFilter(o.attachedSourceFilter),
		attachedSampleGrabber(o.attachedSampleGrabber),
		setMediaType(o.setMediaType),
		attachedRenderer(o.attachedRenderer),
		configuredSourceOutput(o.configuredSourceOutput),
		attachedControl(o.attachedControl),
		setOutputFormat(o.setOutputFormat),
		devicePath(NULL),
		deviceName(NULL)
	{
		InitializeCriticalSection(&(this->mutex));
		if (this->moniker != NULL) {
			this->moniker->AddRef();
			/* ensure that the variables are set in case of device removal */
			this->getName();
			this->getPath();
		}
		if (this->graph != NULL) this->graph->AddRef();
		if (this->capture != NULL) this->capture->AddRef();
		if (this->control != NULL) this->control->AddRef();
		if (this->sourceFilter != NULL) this->sourceFilter->AddRef();
		if (this->nullRenderer != NULL) this->nullRenderer->AddRef();
		if (this->sampleGrabberFilter != NULL) this->sampleGrabberFilter->AddRef();
		if (this->sampleGrabber != NULL) this->sampleGrabber->AddRef();
		if (this->callbackHandler != NULL) this->callbackHandler->AddRef();
	}

	/**
	 * Destructor.
	 */
	virtual ~NativeCaptureDevice() {
		this->stop();
		if (this->moniker != NULL) this->moniker->Release();
		if (this->graph != NULL) this->graph->Release();
		if (this->capture != NULL) this->capture->Release();
		if (this->control != NULL) this->control->Release();
		if (this->sourceFilter != NULL) this->sourceFilter->Release();
		if (this->nullRenderer != NULL) this->nullRenderer->Release();
		if (this->sampleGrabberFilter != NULL) this->sampleGrabberFilter->Release();
		if (this->sampleGrabber != NULL) this->sampleGrabber->Release();
		if (this->callbackHandler != NULL) this->callbackHandler->Release();
		if (this->devicePath != NULL) free(this->devicePath);
		if (this->deviceName != NULL) free(this->deviceName);
		DeleteCriticalSection(&(this->mutex));
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
			if (this->moniker != NULL) this->moniker->Release();
			if (this->graph != NULL) this->graph->Release();
			if (this->capture != NULL) this->capture->Release();
			if (this->control != NULL) this->control->Release();
			if (this->sourceFilter != NULL) this->sourceFilter->Release();
			if (this->nullRenderer != NULL) this->nullRenderer->Release();
			if (this->sampleGrabberFilter != NULL) this->sampleGrabberFilter->Release();
			if (this->sampleGrabber != NULL) this->sampleGrabber->Release();
			if (this->callbackHandler != NULL) this->callbackHandler->Release();
			if (this->devicePath != NULL) free(this->devicePath);
			if (this->deviceName != NULL) free(this->deviceName);
			this->Base::operator= (o);
			this->moniker = o.moniker;
			this->graph = o.graph;
			this->capture = o.capture;
			this->control = o.control;
			this->sourceFilter = o.sourceFilter;
			this->nullRenderer = o.nullRenderer;
			this->sampleGrabberFilter = o.sampleGrabberFilter;
			this->sampleGrabber = o.sampleGrabber;
			this->callbackHandler = o.callbackHandler;
			this->attachedSourceFilter = o.attachedSourceFilter;
			this->attachedSampleGrabber = o.attachedSampleGrabber;
			this->setMediaType = o.setMediaType;
			this->attachedRenderer = o.attachedRenderer;
			this->configuredSourceOutput = o.configuredSourceOutput;
			this->attachedControl = o.attachedControl;
			this->setOutputFormat = o.setOutputFormat;
			this->devicePath = NULL;
			this->deviceName = NULL;
			if (this->moniker != NULL) {
				this->moniker->AddRef();
				/* ensure that the variables are set in case of device removal */
				this->getName();
				this->getPath();
			}
			if (this->graph != NULL) this->graph->AddRef();
			if (this->capture != NULL) this->capture->AddRef();
			if (this->control != NULL) this->control->AddRef();
			if (this->sourceFilter != NULL) this->sourceFilter->AddRef();
			if (this->nullRenderer != NULL) this->nullRenderer->AddRef();
			if (this->sampleGrabberFilter != NULL) this->sampleGrabberFilter->AddRef();
			if (this->sampleGrabber != NULL) this->sampleGrabber->AddRef();
			if (this->callbackHandler != NULL) this->callbackHandler->AddRef();
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
		if (this->devicePath == NULL) {
			this->devicePath = this->getMonikerProperty(L"DevicePath");
		}
		return this->devicePath;
	}

	/**
	 * Returns the human readable name of the capture device. The returned pointer shell not be freed.
	 *
	 * @return capture device name
	 * @remarks The function is not re-entrant safe.
	 */
	virtual const char * getName() {
		if (this->deviceName == NULL) {
			this->deviceName = this->getMonikerProperty(L"Description", L"FriendlyName");
		}
		return this->deviceName;
	}

	/**
	 * Opens a window to configure the capture device.
	 *
	 * @param[in,out] wnd - use this parent window
	 */
	virtual void configure(HWND wnd) {
		this->configureSourceFilter(wnd);
	}

	/**
	 * Returns the current configuration of the capture device. The returned pointer needs to be freed.
	 *
	 * @return capture device configuration
	 */
	virtual char * getConfiguration() {
		// @todo
		// https://docs.microsoft.com/en-us/windows/win32/directshow/displaying-a-filters-property-pages
		// https://docs.microsoft.com/en-us/previous-versions/ms784398%28v%3dvs.85%29
		// https://docs.microsoft.com/en-us/previous-versions/ms783797(v%3Dvs.85)
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
	 */
	virtual bool start(HWND wnd, CaptureCallback & cb) {
		HRESULT res;
		LONGLONG start = 0, stop = MAXLONGLONG;

		/* single concurrent function entry */
		if ( ! TryEnterCriticalSection(&(this->mutex)) ) return false;
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });

		/* fresh start */
		if (this->stopInternal() < 0) return false;
		this->Base::callback = &cb;
		if (this->callbackHandler != NULL) {
			this->callbackHandler->setCallback(cb);
		}

		/* create the `FilterGraph` */
		if (this->graph == NULL) {
			res = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IFilterGraph2, reinterpret_cast<void **>(&(this->graph)));
			if (res != S_OK) return false;
		}

		/* create the `CaptureGraphBuilder` */
		if (this->capture == NULL) {
			res = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, reinterpret_cast<void **>(&(this->capture)));
			if (res != S_OK) return false;
			this->capture->SetFiltergraph(this->graph);
		}

		/* attach the source filter */
		if ( ! this->attachedSourceFilter ) {
			res = this->graph->AddSourceFilterForMoniker(this->moniker, NULL, L"Source Filter", &(this->sourceFilter));
			if (res != S_OK) return false;
			this->attachedSourceFilter = true;
		}

		/* get the controller for the graph */
		if (this->control == NULL) {
			res = this->graph->QueryInterface(IID_IMediaControl, reinterpret_cast<void **>(&(this->control)));
			if (res != S_OK) return false;
		}

		/* set the sample grabber */
		if (this->sampleGrabberFilter == NULL) {
			res = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void **>(&(this->sampleGrabberFilter)));
			if (res != S_OK) return false;
		}

		/* attach the sample grabber */
		if ( ! this->attachedSampleGrabber ) {
			res = this->graph->AddFilter(this->sampleGrabberFilter, L"Sample Grabber");
			if (res != S_OK) return false;
			this->attachedSampleGrabber = true;
		}

		/* configure grabber output */
		if (this->sampleGrabber == NULL) {
			res = this->sampleGrabberFilter->QueryInterface(IID_ISampleGrabber, reinterpret_cast<void**>(&(this->sampleGrabber)));
			if (res != S_OK) return false;
		}

		/* set the output media type */
		if ( ! this->setMediaType ) {
			AM_MEDIA_TYPE mt;
			memset(&mt, 0, sizeof(AM_MEDIA_TYPE));
			mt.majortype = MEDIATYPE_Video;
			mt.subtype = MEDIASUBTYPE_RGB24;
			mt.formattype = FORMAT_VideoInfo;
			res = this->sampleGrabber->SetMediaType(&mt);
			if (res != S_OK) return false;
			this->setMediaType = true;
		}

		/* attach callback handler to grabber */
		if (this->callbackHandler == NULL) {
			this->callbackHandler = new CallbackHandler(*(this->Base::callback));
			this->sampleGrabber->SetCallback(this->callbackHandler, 0 /* use SampleCB */);
		}

		/* create the `NullRenderer` */
		if (this->nullRenderer == NULL) {
			res = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<void **>(&(this->nullRenderer)));
			if (res != S_OK) return false;
			this->graph->AddFilter(this->nullRenderer, L"Null Renderer");
		}

		/* connect graph with current source filter */
		if ( ! this->attachedRenderer ) {
			res = this->capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, this->sourceFilter, this->sampleGrabberFilter, this->nullRenderer);
			if (res != S_OK) return false;
			this->attachedRenderer = true;
		}

		/* configure video capture output format */
		if ( ! this->configuredSourceOutput ) {
			res = this->configureSourceOutput(wnd);
			if (res != S_OK && res != S_FALSE) return false;
			this->configuredSourceOutput = true;
		}

		/* start streaming */
		if ( ! this->attachedControl ) {
			res = this->capture->ControlStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, this->sourceFilter, &start, &stop, CTRL_STARTED, CTRL_STOPPED);
			if (res != S_OK && res != S_FALSE) return false;
			this->attachedControl = true;
		}

		/* get output resolution and format */
		if ( ! this->setOutputFormat ) {
			AM_MEDIA_TYPE mt;
			res = this->sampleGrabber->GetConnectedMediaType(&mt);
			if (res != S_OK) return false;
			if (mt.formattype == FORMAT_VideoInfo && mt.cbFormat >= sizeof(VIDEOINFOHEADER) && mt.pbFormat != NULL) {
				pcf::color::ColorFormat::Type newCf = pcf::color::ColorFormat::UNKNOWN;
				if (mt.subtype == MEDIASUBTYPE_RGB24) {
					newCf = pcf::color::ColorFormat::BGR_24;
				} else {
					return VFW_E_INVALIDMEDIATYPE;
				}
				const VIDEOINFOHEADER * videoInfoHeader = reinterpret_cast<const VIDEOINFOHEADER *>(mt.pbFormat);
				this->callbackHandler->setOutputFormat(newCf, size_t(videoInfoHeader->bmiHeader.biWidth), size_t(videoInfoHeader->bmiHeader.biHeight));
				this->setOutputFormat = true;
			}
			FreeMediaType(mt);
		}

		res = this->control->Run();
		if (res == S_FALSE) {
			/* wait until state changed to running */
			FILTER_STATE fs;
			this->control->GetState(5000, reinterpret_cast<OAFilterState *>(&fs));
			if (fs != State_Running) return false;
		} else if (res != S_OK) {
			return false;
		}

		this->Base::running = true;
		return true;
	}

	/**
	 * Stops the video capture procedure.
	 *
	 * @return true on success, else false
	 */
	virtual bool stop() {
		if ( ! TryEnterCriticalSection(&(this->mutex)) ) return false;
		const auto unlockCsOnReturn = makeScopeExit([this]() { LeaveCriticalSection(&(this->mutex)); });
		return this->stopInternal() >= 1;
	}
private:
	/**
	 * Returns the UTF-8 string of a property field in the configured
	 * video capture device descriptor. An alternative property field
	 * can be given which is used of the first one does not exist.
	 *
	 * @param[in] field1 - property field to read
	 * @param[in] field2 - optional alternative property field to read
	 * @return associated UTF-8 from the property field
	 */
	char * getMonikerProperty(const wchar_t * field1, const wchar_t * field2 = NULL) const {
		char * result = NULL;
		if (this->moniker == NULL || field1 == NULL) return result;
		IPropertyBag * propBag;
		HRESULT res = this->moniker->BindToStorage(0, 0, IID_PPV_ARGS(&propBag));
		if ( FAILED(res) ) return result;
		VARIANT var;
		VariantInit(&var);
		res = propBag->Read(field1, &var, 0);
        if (FAILED(res) && field2 != NULL) {
			res = propBag->Read(field2, &var, 0);
        }
        if ( SUCCEEDED(res) ) {
			result = cvutf8_fromUtf16(var.bstrVal);
        }
		VariantClear(&var);
		propBag->Release();
		return result;
	}

	/**
	 * Opens a configuration window for the configured video capture device.
	 * This enables the user to configure the current video capture device
	 * accordingly.
	 *
	 * @param[in] parentWnd - associated parent window handle
	 */
	void configureSourceFilter(HWND parentWnd) {
		if (this->sourceFilter == NULL) return;
		ISpecifyPropertyPages * propPages = NULL;
		IUnknown * filterUnk = NULL;
		FILTER_INFO filterInfo;
		bool hasFilterInfo = false;
		CAUUID caGUID;
		bool hasCaGUID = false;
		const auto cleanupOnReturn = makeScopeExit([&]() {
			if (propPages != NULL) propPages->Release();
			if (filterUnk != NULL) filterUnk->Release();
			if (hasFilterInfo && filterInfo.pGraph != NULL) filterInfo.pGraph->Release();
			if (hasCaGUID && caGUID.pElems != NULL) CoTaskMemFree(caGUID.pElems);
		});
		HRESULT res = this->sourceFilter->QueryInterface(IID_ISpecifyPropertyPages, reinterpret_cast<void **>(&propPages));
		if ( FAILED(res) ) return;
		res = this->sourceFilter->QueryFilterInfo(&filterInfo);
		if ( FAILED(res) ) return;
		hasFilterInfo = true;
		res = this->sourceFilter->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&filterUnk));
		if ( FAILED(res) ) return;
		propPages->GetPages(&caGUID);
		hasCaGUID = true;
		OleCreatePropertyFrame(
			parentWnd,           /* Parent window */
			0, 0,                /* Reserved */
			filterInfo.achName,  /* Caption for the dialog box */
			1,                   /* Number of objects (just the filter) */
			&filterUnk,          /* Array of object pointers. */
			caGUID.cElems,       /* Number of property pages */
			caGUID.pElems,       /* Array of property page CLSIDs */
			0,                   /* Locale identifier */
			0, NULL              /* Reserved */
		);
	}

	/**
	 * Opens a modal window which requires the user to select the
	 * desired video capture source parameters.
	 *
	 * @param[in] parentWnd - associated parent window handle
	 * @return HRESULT value depending on the implementation
	 */
	HRESULT configureSourceOutput(HWND parentWnd) {
		if (this->sourceFilter == NULL) return E_POINTER;
		IEnumPins * pinEnum = NULL;
		IPin * pin = NULL;
		PIN_INFO pinInfo;
		bool hasPinInfo = false;
		ISpecifyPropertyPages * propPages = NULL;
		IUnknown * filterUnk = NULL;
		CAUUID caGUID;
		bool hasCaGUID = false;
		BOOL connected;
		HRESULT res = S_OK;
		const auto cleanupOnReturn = makeScopeExit([&]() {
			if (pinEnum != NULL) pinEnum->Release();
			if (pin != NULL) pin->Release();
			if (hasPinInfo && pinInfo.pFilter != NULL) pinInfo.pFilter->Release();
			if (propPages != NULL) propPages->Release();
			if (filterUnk != NULL) filterUnk->Release();
			if (hasCaGUID && caGUID.pElems != NULL) CoTaskMemFree(caGUID.pElems);
		});
		res = this->sourceFilter->EnumPins(&pinEnum);
		if ( FAILED(res) ) return res;
		while (pinEnum->Next(1, &pin, NULL) == S_OK) {
			const auto cleanupOnNext1 = makeScopeExit([&]() {
				pin->Release();
				pin = NULL;
			});
			res = IsPinConnected(pin, &connected);
			if ( FAILED(res) ) return res;
			if (connected != TRUE) continue;
			res = pin->QueryPinInfo(&pinInfo);
			if ( FAILED(res) ) return res;
			hasPinInfo = true;
			const auto cleanupOnNext2 = makeScopeExit([&]() {
				if (hasPinInfo && pinInfo.pFilter != NULL) {
					pinInfo.pFilter->Release();
					pinInfo.pFilter = NULL;
					hasPinInfo = false;
				}
			});
			if (pinInfo.dir != PINDIR_OUTPUT) continue;
			res = pin->QueryInterface(IID_ISpecifyPropertyPages, reinterpret_cast<void **>(&propPages));
			if ( FAILED(res) ) return res;
			res = pin->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&filterUnk));
			if ( FAILED(res) ) return res;
			propPages->GetPages(&caGUID);
			hasCaGUID = true;
			return OleCreatePropertyFrame(
				parentWnd,       /* Parent window (this makes the new window modal) */
				0, 0,            /* Reserved */
				pinInfo.achName, /* Caption for the dialog box */
				1,               /* Number of objects (just the filter) */
				&filterUnk,      /* Array of object pointers. */
				caGUID.cElems,   /* Number of property pages */
				caGUID.pElems,   /* Array of property page CLSIDs */
				0,               /* Locale identifier */
				0, NULL          /* Reserved */
			);
		}
		return S_FALSE;
	}

	/**
	 * Stop running video capturing.
	 *
	 * @return true on success, else false
	 * @remarks The caller is expected to hold the mutex lock.
	 */
	inline int stopInternal() {
		HRESULT res;
		if ( ! this->isRunning() ) return 0;
		if (this->control == NULL) return 0;
		res = this->control->StopWhenReady();
		if (res != S_OK) return -1;
		return 1;
	}
};


} /* anonymous namespace */


struct NativeVideoCaptureProvider::Pimple {
	static thread_local size_t initCount;

	static inline bool initialize() {
		if (NativeVideoCaptureProvider::Pimple::initCount > 0) {
			NativeVideoCaptureProvider::Pimple::initCount++;
			return true;
		}
		HRESULT res = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if ( FAILED(res) ) return false;
		NativeVideoCaptureProvider::Pimple::initCount++;
		res = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
		if ( FAILED(res) ) return false;
		return true;
	}

	static inline bool uninitialize() {
		if (NativeVideoCaptureProvider::Pimple::initCount <= 0) return false;
		NativeVideoCaptureProvider::Pimple::initCount--;
		if (NativeVideoCaptureProvider::Pimple::initCount <= 0) CoUninitialize();
		return true;
	}
};


thread_local size_t NativeVideoCaptureProvider::Pimple::initCount = 0;


NativeVideoCaptureProvider::NativeVideoCaptureProvider():
	self(NULL)
{
	if ( ! NativeVideoCaptureProvider::Pimple::initialize() ) {
		throw std::runtime_error("Windows COM initialization failed");
	}
}


NativeVideoCaptureProvider::~NativeVideoCaptureProvider() {
	if (self != NULL) delete self;
	NativeVideoCaptureProvider::Pimple::uninitialize();
}


CaptureDeviceList NativeVideoCaptureProvider::getDeviceList() {
	HRESULT res;
	ICreateDevEnum * devEnum = NULL;
	IEnumMoniker * monikerEnum = NULL;
	IMoniker * moniker;
	CaptureDeviceList result;

	const auto onReturn = makeScopeExit([&]{
		if (devEnum != NULL) devEnum->Release();
		if (monikerEnum != NULL) monikerEnum->Release();
	});

	/* instantiate enumerator for video input devices */
	res = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void **>(&devEnum));
	if ( FAILED(res) ) return result;

	res = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikerEnum, 0);
	if (FAILED(res) || res == S_FALSE) return result;

	/* get video input device list items */
	while (monikerEnum->Next(1, &moniker, NULL) == S_OK) {
		result.push_back(new NativeCaptureDevice(moniker));
		moniker->Release();
	}

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
