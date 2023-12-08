/**
 * @file CaptureDirectShow.ipp
 * @author Daniel Starke
 * @date 2019-10-03
 * @version 2023-12-08
 * @todo rework with new MF API:
 *  - https://www.dreamincode.net/forums/topic/347938-a-new-webcam-api-tutorial-in-c-for-windows/
 *  - https://www.codeproject.com/Articles/776058/Capturing-Live-video-from-Web-camera-on-Windows-an
 */
#include <stdexcept>
#include <libpcf/cvutf8.h>
#include <libpcf/tchar.h>
#include <pcf/video/Capture.hpp>
#include <pcf/ScopeExit.hpp>
#include <pcf/UtilityWindows.hpp>
#include <windows.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <qedit.h>
#include <dbt.h>
#include <ks.h>


/* bring `ComPtr` into scope */
using namespace Microsoft::WRL;


#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif /* GCC */


#ifndef MAXLONGLONG
#define MAXLONGLONG 0x7FFFFFFFFFFFFFFF
#endif


/** Capture stream control value forwarded if started. */
#define CTRL_STARTED 1
/** Capture stream control value forwarded if stopped. */
#define CTRL_STOPPED 2


#define FILTER_NAME L"VKVM Capture Sink"
#define PIN_NAME L"VKVM Capture Sink Pin"


namespace pcf {
namespace video {
namespace {


/**
 * Copies a media type structure to another variable.
 *
 * @param[out] dst - destination variable
 * @param[in] src - source variable
 * @return true on success, false on allocation error
 */
inline bool CopyMediaType(AM_MEDIA_TYPE & dst, const AM_MEDIA_TYPE & src) {
	memcpy(&dst, &src, sizeof(AM_MEDIA_TYPE));
	if (src.cbFormat > 0 && src.pbFormat != NULL) {
		dst.pbFormat = static_cast<PBYTE>(CoTaskMemAlloc(src.cbFormat));
		if (dst.pbFormat == NULL) {
			dst.cbFormat = 0;
			return false;
		}
		memcpy(dst.pbFormat, src.pbFormat, src.cbFormat);
	}
	if (dst.pUnk != NULL) dst.pUnk->AddRef();
	return true;
}


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
 * Holds the reference counter, the weak reference counter
 * and the reference to the managed object. Does not provide
 * `std::weak_ptr` functions. All managing operations need
 * to be performed outside of this class. The managed object
 * needs to call `releaseWeak()` upon destruction.
 *
 * @tparam T - type of the managed object
 */
template <typename T>
class WeakComRef {
public:
	T & ref; /**< referneced COM object */
private:
	ULONG refCount; /**< number of references to `ref` */
	ULONG weakCount; /**< number of weak references + 1 if `refCount > 0` */
public:
	/**
	 * Constructor.
	 *
	 * @param[in,out] r - managed object
	 */
	explicit inline WeakComRef(T & r):
		ref(r),
		refCount(1),
		weakCount(1)
	{}

	/** Destructor. */
	inline ~WeakComRef() {}

	/**
	 * Increases the reference counter.
	 *
	 * @return previous reference counter value
	 */
	inline ULONG acquire() {
		return InterlockedIncrement(&(this->refCount));
	}

	/**
	 * Decreases the reference counter.
	 *
	 * @return previous reference counter value
	 */
	inline ULONG release() {
		return InterlockedDecrement(&(this->refCount));
	}

	/**
	 * Increases the weak reference counter.
	 *
	 * @return previous value
	 */
	inline ULONG acquireWeak() {
		return InterlockedIncrement(&(this->weakCount));
	}

	/**
	 * Decreases the weak reference counter.
	 * The `WeakComRef` object is deleted once this reaches 0.
	 *
	 * @return previous value
	 */
	inline ULONG releaseWeak() {
		const ULONG count = InterlockedDecrement(&(this->weakCount));
		if (count == 0) delete this;
		return count;
	}

	/**
	 * Tries to acquire a ComPtr to the managed object.
	 *
	 * @return ComPtr to the managed object if available
	 */
	inline ComPtr<T> lock() noexcept {
		const ULONG oldRefCount = InterlockedIncrementIfNonZero(&(this->refCount));
		if (oldRefCount == 0) return ComPtr<T>();
		ComPtr<T> res(&(this->ref));
		InterlockedDecrement(&(this->refCount));
		return res;
	}
private:
	/**
	 * Increments the pointed variable if non-zero.
	 *
	 * @param[in,out] ptr - pointed variable to increment
	 * @return previous value or zero if zero
	 */
	static inline ULONG InterlockedIncrementIfNonZero(volatile ULONG * ptr) {
		ULONG old;
		do {
			old = *ptr;
			if (old == 0) return 0;
		} while (ULONG(InterlockedCompareExchange(ptr, old + 1, old)) != old);
		return old;
	}
};


/**
 * Implements an enumerator over a single media type via `IEnumMediaTypes` interface.
 */
class MediaTypeEnumerator : public IEnumMediaTypes {
private:
	AM_MEDIA_TYPE mediaType; /**< media type used for enumeration */
	UINT index; /**< enumeration index */
	ULONG refCount; /**< reference counter for automatic deletion */
public:
	/**
	 * Constructor.
	 *
	 * @param[in] mt - media type used for enumeration
	 */
	explicit inline MediaTypeEnumerator(AM_MEDIA_TYPE & mt):
		index(0),
		refCount(1)
	{
		CopyMediaType(this->mediaType, mt);
	}

	/** Destructor. */
	inline virtual ~MediaTypeEnumerator() {
		FreeMediaType(this->mediaType);
	}

	/* `IUnknown` methods */
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObj) {
		if (ppvObj == NULL) return E_POINTER;
		if (riid == IID_IUnknown) {
			*ppvObj = static_cast<IUnknown *>(this);
			this->AddRef();
			return S_OK;
		} else if (riid == IID_IEnumMediaTypes) {
			*ppvObj = static_cast<IEnumMediaTypes *>(this);
			this->AddRef();
			return S_OK;
		}
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&(this->refCount));
	}

	virtual ULONG STDMETHODCALLTYPE Release() {
		const ULONG count = InterlockedDecrement(&(this->refCount));
		if (count == 0) delete this;
		return count;
	}

	/* `IEnumMediaTypes` methods */
	virtual HRESULT STDMETHODCALLTYPE Next(ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes, ULONG * pcFetched) {
		if (ppMediaTypes == NULL) return E_POINTER;
		UINT nFetched = 0;
		if (this->index == 0 && cMediaTypes > 0) {
			*ppMediaTypes = static_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
			if (*ppMediaTypes != NULL) CopyMediaType(**ppMediaTypes, this->mediaType);
			this->index++;
			nFetched = 1;
		}
		if ( pcFetched ) *pcFetched = nFetched;
		return (nFetched == cMediaTypes) ? S_OK : S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG cMediaTypes) {
		this->index += cMediaTypes;
		return (this->index > 1) ? S_FALSE : S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Reset() {
		this->index = 0;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes ** ppEnum) {
		if (ppEnum == NULL) return E_POINTER;
		*ppEnum = new MediaTypeEnumerator(this->mediaType);
		return (*ppEnum == NULL) ? E_OUTOFMEMORY : S_OK;
	}
};


/**
 * Implements an enumerator over a single pin via `IEnumPins` interface.
 */
class PinEnumerator : public IEnumPins {
	ComPtr<IPin> pin;
	UINT index;
	ULONG refCount;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] p - pin used for enumeration
	 */
	explicit inline PinEnumerator(const ComPtr<IPin> & p):
		pin(p),
		index(0),
		refCount(1)
	{}

	/** Destructor. */
	inline virtual ~PinEnumerator() {}

	/* `IUnknown` methods */
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObj) {
		if (ppvObj == NULL) return E_POINTER;
		if (riid == IID_IUnknown) {
			*ppvObj = static_cast<IUnknown *>(this);
			this->AddRef();
			return S_OK;
		} else if (riid == IID_IEnumPins) {
			*ppvObj = static_cast<IEnumPins *>(this);
			this->AddRef();
			return S_OK;
		}
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&(this->refCount));
	}

	virtual ULONG STDMETHODCALLTYPE Release() {
		const ULONG count = InterlockedDecrement(&(this->refCount));
		if (count == 0) delete this;
		return count;
	}

	/* `IEnumPins` methods */
	virtual HRESULT STDMETHODCALLTYPE Next(ULONG cPins, IPin ** ppPins, ULONG * pcFetched) {
		if (ppPins == NULL) return E_POINTER;
		UINT nFetched = 0;
		if (this->index == 0 && cPins > 0) {
			*ppPins = this->pin;
			this->pin->AddRef();
			this->index++;
			nFetched = 1;
		}
		if ( pcFetched ) *pcFetched = nFetched;
		return (nFetched == cPins) ? S_OK : S_FALSE;
	}

	virtual HRESULT STDMETHODCALLTYPE Skip(ULONG cPins) {
		this->index += cPins;
		return (this->index > 1) ? S_FALSE : S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Reset() {
		this->index = 0;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Clone(IEnumPins ** ppEnum) {
		if (ppEnum == NULL) return E_POINTER;
		*ppEnum = new PinEnumerator(this->pin);
		return (*ppEnum == NULL) ? E_OUTOFMEMORY : S_OK;
	}
};


/* forward declaration */
class InputPin;


/**
 * Implementation of the `IBaseFilter` interface to process frames and forward them to
 * `CaptureCallback`. It is implemented as renderer rather than pass-through filter to
 * allow frame rate adjustment feedback via `IQualityControl` interface of the connected
 * output pin.
 */
class CaptureSink : public IBaseFilter, public IAMFilterMiscFlags {
public:
	FILTER_STATE state; /**< filter running state */
	ComPtr<IFilterGraph> graph; /**< associated filter graph */
	ComPtr<InputPin> pin; /**< filter input pin */
	WeakComRef<CaptureSink> * self; /**< reference counter object */
public:
	explicit inline CaptureSink(CaptureCallback & cb);
	inline virtual ~CaptureSink();
	inline void setCallback(CaptureCallback & cb);
	/* `IUnknown` methods */
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID * ppvObj);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();
	/* `IPersist` methods */
	virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID * pClsID);
	/* `IMediaFilter` methods */
	virtual HRESULT STDMETHODCALLTYPE GetState(DWORD dwMSecs, FILTER_STATE * State);
	virtual HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock * pClock);
	virtual HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock ** pClock);
	virtual HRESULT STDMETHODCALLTYPE Stop();
	virtual HRESULT STDMETHODCALLTYPE Pause();
	virtual HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart);
	/* `IBaseFilter` methods */
	virtual HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins ** ppEnum);
	virtual HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, IPin ** ppPin);
	virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO * pInfo);
	virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);
	virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR * pVendorInfo);
	/* `IAMFilterMiscFlags` methods */
	virtual ULONG STDMETHODCALLTYPE GetMiscFlags();
};


/**
 * Implements the pin interfaces `IPin` and `IMemInputPin` for
 * `CaptureSink`. This ensures correct target format, creates
 * feedback loop to the capture source for frame rate adjustment
 * and forwards the received samples as captures frames to the
 * registered VKVM callback.
 */
class InputPin : public IPin, public IMemInputPin {
private:
	WeakComRef<CaptureSink> & filter; /**< weak referenced filter */
	ComPtr<IPin> connectedPin; /**< connected pin which produces the input for this filter */
	ComPtr<IQualityControl> inputQuality; /**< input quality control sink */
	ComPtr<IReferenceClock> clock; /**< refernce clock used */
	REFERENCE_TIME offset; /**< real time to stream time offset */
	bool hasOffset; /**< has real time to stream time offset? */
	LONG proportion; /**< desired input frame in 0.1% */
	CaptureCallback * callback; /**< associated VKVM capture callback handle */
	pcf::color::ColorFormat::Type colorFormat; /**< color format of the captured frames */
	size_t width; /**< width of the captured frames */
	size_t height; /**< height of the captured frames */
	AM_MEDIA_TYPE mediaType; /**< connected input media type */
	bool flushing; /**< ignore samples until end of flushing? */
	ULONG refCount; /**< reference counter for automatic deletion */
public:
	/**
	 * Constructor.
	 *
	 * @param[in,out] cb - VKVM capture callback handle
	 * @param[in] f - weak referenced filter
	 */
	explicit inline InputPin(CaptureCallback & cb, WeakComRef<CaptureSink> & f):
		filter(f),
		hasOffset(false),
		proportion(1000),
		callback(&cb),
		colorFormat(pcf::color::ColorFormat::UNKNOWN),
		width(0),
		height(0),
		flushing(false),
		refCount(1)
	{
		memset(&(this->mediaType), 0, sizeof(AM_MEDIA_TYPE));
		this->mediaType.majortype = MEDIATYPE_Video;
		this->mediaType.subtype = MEDIASUBTYPE_RGB24;
		this->mediaType.formattype = FORMAT_VideoInfo;
		this->mediaType.pbFormat = static_cast<PBYTE>(CoTaskMemAlloc(sizeof(VIDEOINFOHEADER)));
		if (this->mediaType.pbFormat != NULL) {
			this->mediaType.cbFormat = ULONG(sizeof(VIDEOINFOHEADER));
			VIDEOINFOHEADER * vih = reinterpret_cast<VIDEOINFOHEADER *>(this->mediaType.pbFormat);
			memset(vih, 0, sizeof(VIDEOINFOHEADER));
			vih->bmiHeader.biSize = DWORD(sizeof(BITMAPINFOHEADER));
			vih->bmiHeader.biWidth = 1920;
			vih->bmiHeader.biHeight = 1080;
			vih->bmiHeader.biPlanes = 1;
			vih->bmiHeader.biCompression = BI_RGB;
		}
		this->filter.acquireWeak();
	}

	/** Destructor. */
	inline virtual ~InputPin() {
		FreeMediaType(this->mediaType);
		this->filter.releaseWeak();
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
	 * Retries the current reference clock.
	 *
	 * @return current reference clock
	 */
	inline ComPtr<IReferenceClock> getClock() const {
		return this->clock;
	}

	/**
	 * Sets a new reference clock.
	 *
	 * @param[in] newClock - new reference clock
	 */
	inline void setClock(const ComPtr<IReferenceClock> & newClock) {
		this->clock = newClock;
		this->hasOffset = false;
	}

	/* `IUnknown` methods */
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObj) {
		if (ppvObj == NULL) return E_POINTER;
		if (riid == IID_IUnknown) {
			*ppvObj = static_cast<IUnknown *>(static_cast<IPin *>(this));
			this->AddRef();
			return S_OK;
		} else if (riid == IID_IPin) {
			*ppvObj = static_cast<IPin *>(this);
			this->AddRef();
			return S_OK;
		} else if (riid == IID_IMemInputPin) {
			*ppvObj = static_cast<IMemInputPin *>(this);
			this->AddRef();
			return S_OK;
		}
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef() {
		return InterlockedIncrement(&(this->refCount));
	}

	virtual ULONG STDMETHODCALLTYPE Release() {
		const ULONG count = InterlockedDecrement(&(this->refCount));
		if (count == 0) delete this;
		return count;
	}

	/* `IPin` methods */
	virtual HRESULT STDMETHODCALLTYPE Connect(IPin * /* pReceivePin */, const AM_MEDIA_TYPE * pmt) {
		const ComPtr<CaptureSink> parent = this->filter.lock();
		if ( ! parent ) return S_FALSE;
		if (parent->state == State_Running) return VFW_E_NOT_STOPPED;
		if ( this->connectedPin ) return VFW_E_ALREADY_CONNECTED;
		if (pmt == NULL) return S_OK;
		if (pmt->majortype != GUID_NULL && pmt->majortype != MEDIATYPE_Video) return S_FALSE;
		if (pmt->formattype == FORMAT_VideoInfo && pmt->pbFormat != NULL) {
			const VIDEOINFOHEADER * vih = reinterpret_cast<const VIDEOINFOHEADER *>(pmt->pbFormat);
			if (pmt->subtype != MEDIASUBTYPE_RGB24 || vih->bmiHeader.biWidth == 0 || vih->bmiHeader.biHeight == 0) return VFW_E_INVALIDMEDIATYPE;
			this->colorFormat = pcf::color::ColorFormat::BGR_24;
			this->width = size_t(vih->bmiHeader.biWidth);
			this->height = size_t(vih->bmiHeader.biHeight);
		} else if (pmt->formattype == FORMAT_VideoInfo2 && pmt->pbFormat != NULL) {
			const VIDEOINFOHEADER2 * vih = reinterpret_cast<const VIDEOINFOHEADER2 *>(pmt->pbFormat);
			if (pmt->subtype != MEDIASUBTYPE_RGB24 || vih->bmiHeader.biWidth == 0 || vih->bmiHeader.biHeight == 0) return VFW_E_INVALIDMEDIATYPE;
			this->colorFormat = pcf::color::ColorFormat::BGR_24;
			this->width = size_t(vih->bmiHeader.biWidth);
			this->height = size_t(vih->bmiHeader.biHeight);
		} else {
			return VFW_E_INVALIDMEDIATYPE;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE * pmt) {
		const ComPtr<CaptureSink> parent = this->filter.lock();
		if ( ! parent ) return S_FALSE;
		if (parent->state != State_Stopped) return VFW_E_NOT_STOPPED;
		if ( this->connectedPin ) return VFW_E_ALREADY_CONNECTED;
		if (pConnector == NULL || pmt == NULL) return E_POINTER;
		if (this->QueryAccept(pmt) != S_OK) return VFW_E_TYPE_NOT_ACCEPTED;
		this->connectedPin = pConnector;
		this->connectedPin.As(&(this->inputQuality));
		FreeMediaType(this->mediaType);
		if ( ! CopyMediaType(this->mediaType, *pmt) ) return E_OUTOFMEMORY;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Disconnect() {
		if ( ! this->connectedPin ) return S_FALSE;
		this->connectedPin.Reset();
		this->inputQuality.Reset();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ConnectedTo(IPin ** pPin) {
		if (pPin == NULL) return E_POINTER;
		if ( ! this->connectedPin ) return VFW_E_NOT_CONNECTED;
		this->connectedPin->AddRef();
		*pPin = this->connectedPin;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE * pmt) {
		if (pmt == NULL) return E_POINTER;
		if ( ! this->connectedPin ) return VFW_E_NOT_CONNECTED;
		return CopyMediaType(*pmt, this->mediaType) ? S_OK : E_OUTOFMEMORY;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO * pInfo) {
		const ComPtr<CaptureSink> parent = this->filter.lock();
		if (pInfo == NULL) return E_POINTER;
		if ( parent ) parent->AddRef();
		pInfo->pFilter = parent;
		pInfo->dir = PINDIR_INPUT;
		memcpy(pInfo->achName, FILTER_NAME, sizeof(FILTER_NAME));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION * pPinDir) {
		if (pPinDir == NULL) return E_POINTER;
		*pPinDir = PINDIR_INPUT;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryId(LPWSTR * lpId) {
		if (lpId == NULL) return E_POINTER;
		wchar_t * str = static_cast<wchar_t *>(CoTaskMemAlloc(sizeof(PIN_NAME)));
		if (str == NULL) E_OUTOFMEMORY;
		memcpy(str, PIN_NAME, sizeof(PIN_NAME));
		*lpId = str;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE * pmt) {
		if (pmt == NULL) return S_FALSE;
		if (pmt->majortype != MEDIATYPE_Video) return S_FALSE;
		if (pmt->formattype == FORMAT_VideoInfo && pmt->pbFormat != NULL) {
			const VIDEOINFOHEADER * vih = reinterpret_cast<const VIDEOINFOHEADER *>(pmt->pbFormat);
			if (pmt->subtype != MEDIASUBTYPE_RGB24 || vih->bmiHeader.biWidth == 0 || vih->bmiHeader.biHeight == 0) return S_FALSE;
			this->colorFormat = pcf::color::ColorFormat::BGR_24;
			this->width = size_t(vih->bmiHeader.biWidth);
			this->height = size_t(vih->bmiHeader.biHeight);
		} else if (pmt->formattype == FORMAT_VideoInfo2 && pmt->pbFormat != NULL) {
			const VIDEOINFOHEADER2 * vih = reinterpret_cast<const VIDEOINFOHEADER2 *>(pmt->pbFormat);
			if (pmt->subtype != MEDIASUBTYPE_RGB24 || vih->bmiHeader.biWidth == 0 || vih->bmiHeader.biHeight == 0) return S_FALSE;
			this->colorFormat = pcf::color::ColorFormat::BGR_24;
			this->width = size_t(vih->bmiHeader.biWidth);
			this->height = size_t(vih->bmiHeader.biHeight);
		} else {
			return S_FALSE;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes ** ppEnum) {
		if (ppEnum == NULL) return E_POINTER;
		*ppEnum = new MediaTypeEnumerator(this->mediaType);
		if (*ppEnum == NULL) return E_OUTOFMEMORY;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin ** /* apPin */, ULONG * /* nPin */) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE EndOfStream() {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE BeginFlush() {
		this->flushing = true;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE EndFlush() {
		this->flushing = false;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME /* tStart */, REFERENCE_TIME /* tStop */, double /* dRate */) {
		return S_OK;
	}

	/* `IMemInputPin` methods */
	virtual HRESULT STDMETHODCALLTYPE GetAllocator(IMemAllocator ** /* ppAllocator */) {
		return VFW_E_NO_ALLOCATOR;
	}

	virtual HRESULT STDMETHODCALLTYPE NotifyAllocator(IMemAllocator * /* pAllocator */, BOOL /* bReadOnly */) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(ALLOCATOR_PROPERTIES * /* pProps */) {
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE Receive(IMediaSample * pSample) {
		if (pSample == NULL) return E_POINTER;
		if ( this->flushing ) return S_FALSE;
		if (pSample->IsPreroll() == S_OK || pSample->IsDiscontinuity() == S_OK) return S_OK;
		if (this->callback == NULL) return S_FALSE;

		/* latency handling */
		if (this->clock != NULL && this->inputQuality != NULL) {
			const ComPtr<CaptureSink> parent = this->filter.lock();
			if ( ! parent ) return S_FALSE;
			REFERENCE_TIME start, now;
			if (pSample->GetTime(&start, &now) != VFW_E_SAMPLE_TIME_NOT_SET) {
				if (this->clock->GetTime(&now) != S_OK) return S_OK;
				const REFERENCE_TIME sampleTime = start + this->offset;
				if ( ! this->hasOffset ) {
					/* real time to stream time offset */
					this->offset = now - start;
					this->hasOffset = true;
				} else if (((now - sampleTime) >> ((8 * sizeof(REFERENCE_TIME)) - 1)) != 0) {
					/* fix invalid offset */
					this->offset = now - start;
				} else {
					/* force max. latency by dropping frames at the source filter */
					LONG newProportion = (now <= sampleTime) ? 1000 : LONG(400000000UL / ULONG(now - sampleTime));
					if (newProportion < 200) {
						newProportion = 200;
					} else if (newProportion > 1000) {
						newProportion = 1000;
					}
					Quality q;
					q.Proportion = newProportion;
					q.Type = (newProportion < this->proportion) ? Flood : Famine;
					q.Late = now - sampleTime;
					q.TimeStamp = start;
					this->proportion = newProportion;
					this->inputQuality->Notify(parent, q);
				}
			}
		}

		BYTE * buffer;
		const HRESULT res = pSample->GetPointer(&buffer);
		if (res != S_OK) return res;

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

	virtual HRESULT STDMETHODCALLTYPE ReceiveMultiple(IMediaSample ** pSamples, long nSamples, long * nSamplesProcessed) {
		if (pSamples == NULL) return E_POINTER;
		if ( this->flushing ) return S_FALSE;
		long nProcessed = 0;
		for (long i = 0; i < nSamples; i++) {
			if (this->Receive(pSamples[i]) == S_OK) nProcessed++;
		}
		if (nSamplesProcessed != NULL) *nSamplesProcessed = nProcessed;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ReceiveCanBlock() {
		return S_FALSE;
	}
};


/**
 * Constructor.
 *
 * @param[in,out] cb - VKVM capture callback handle
 */
CaptureSink::CaptureSink(CaptureCallback & cb):
	state(State_Stopped),
	self(new WeakComRef<CaptureSink>(*this))
{
	this->pin.Attach(new InputPin(cb, *(this->self)));
}


/** Destructor. */
CaptureSink::~CaptureSink() {
	self->releaseWeak();
}


/**
 * Set a new VKVM capture callback handle.
 *
 * @param[in] cb - new VKVM capture callback handle
 */
inline void CaptureSink::setCallback(CaptureCallback & cb) {
	this->pin->setCallback(cb);
}


HRESULT STDMETHODCALLTYPE CaptureSink::QueryInterface(REFIID riid, LPVOID * ppvObj) {
	if (ppvObj == NULL) return E_POINTER;
	if (riid == IID_IUnknown) {
		*ppvObj = static_cast<IUnknown *>(static_cast<IBaseFilter *>(this));
		this->AddRef();
		return S_OK;
	} else if (riid == IID_IPersist) {
		*ppvObj = static_cast<IPersist *>(this);
		this->AddRef();
		return S_OK;
	} else if (riid == IID_IMediaFilter) {
		*ppvObj = static_cast<IMediaFilter *>(this);
		this->AddRef();
		return S_OK;
	} else if (riid == IID_IBaseFilter) {
		*ppvObj = static_cast<IBaseFilter *>(this);
		this->AddRef();
		return S_OK;
	} else if (riid == IID_IAMFilterMiscFlags) {
		*ppvObj = static_cast<IAMFilterMiscFlags *>(this);
		this->AddRef();
		return S_OK;
	}
	*ppvObj = NULL;
	return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE CaptureSink::AddRef() {
	ULONG res = self->acquire();
	return res;
}


ULONG STDMETHODCALLTYPE CaptureSink::Release() {
	const ULONG count = self->release();
    if (count == 0) delete this;
    return count;
}


HRESULT STDMETHODCALLTYPE CaptureSink::GetClassID(CLSID * pClsID) {
	return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE CaptureSink::GetState(DWORD /* dwMSecs */, FILTER_STATE * State) {
	if (State == NULL) return E_POINTER;
	*State = this->state;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::SetSyncSource(IReferenceClock * pClock) {
	this->pin->setClock(pClock);
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::GetSyncSource(IReferenceClock ** pClock) {
	if (pClock == NULL) return E_POINTER;
	*pClock = this->pin->getClock();
	(*pClock)->AddRef();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::Stop() {
	this->state = State_Stopped;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::Pause() {
	this->state = State_Paused;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::Run(REFERENCE_TIME /* tStart */) {
	this->state = State_Running;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::EnumPins(IEnumPins ** ppEnum) {
	if (ppEnum == NULL) return E_POINTER;
	*ppEnum = new PinEnumerator(this->pin);
	return (*ppEnum == NULL) ? E_OUTOFMEMORY : S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::FindPin(LPCWSTR Id, IPin ** ppPin) {
	if (Id == NULL || ppPin == NULL) return E_POINTER;
	if (lstrcmpW(Id, PIN_NAME) != 0) {
		ppPin = NULL;
		return VFW_E_NOT_FOUND;
	}
	*ppPin = this->pin;
	this->pin->AddRef();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::QueryFilterInfo(FILTER_INFO * pInfo) {
	if (pInfo == NULL) return E_POINTER;
	memcpy(pInfo->achName, FILTER_NAME, sizeof(FILTER_NAME));
	pInfo->pGraph = this->graph;
	if (this->graph != NULL) this->graph->AddRef();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR /* pName */) {
	this->graph = pGraph;
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CaptureSink::QueryVendorInfo(LPWSTR * /* pVendorInfo */) {
	return E_NOTIMPL;
}


ULONG STDMETHODCALLTYPE CaptureSink::GetMiscFlags() {
	return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}


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
	ComPtr<IMoniker> moniker; /**< video capture device */
	ComPtr<IFilterGraph2> graph; /**< randering graph */
	ComPtr<ICaptureGraphBuilder2> capture; /**< capture graph builder */
	ComPtr<IMediaControl> control; /**< capture control */
	ComPtr<IReferenceClock> referenceClock; /**< reference clock */
	ComPtr<IBaseFilter> sourceFilter; /**< video source filter */
	ComPtr<CaptureSink> renderer; /**< renderer forwarding the final image frames to the VKVM callback */
	bool attachedSourceFilter; /**< source filter was attached to the capture device? */
	bool attachedRenderer; /**< renderer was attached to the capture device? */
	bool createdStream; /**< renderer stream has been built? */
	bool configuredSourceOutput; /**< output format video capture has been configured? */
	bool attachedControl; /**< control interface have been attached to the capture graph? */
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
		attachedSourceFilter(false),
		attachedRenderer(false),
		createdStream(false),
		configuredSourceOutput(false),
		attachedControl(false),
		devicePath(NULL),
		deviceName(NULL)
	{
		if ( this->moniker ) {
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
		referenceClock(o.referenceClock),
		sourceFilter(o.sourceFilter),
		renderer(o.renderer),
		attachedSourceFilter(o.attachedSourceFilter),
		attachedRenderer(o.attachedRenderer),
		createdStream(o.createdStream),
		configuredSourceOutput(o.configuredSourceOutput),
		attachedControl(o.attachedControl),
		devicePath(NULL),
		deviceName(NULL)
	{
		InitializeCriticalSection(&(this->mutex));
		if ( this->moniker ) {
			/* ensure that the variables are set in case of device removal */
			this->getName();
			this->getPath();
		}
	}

	/**
	 * Destructor.
	 */
	virtual ~NativeCaptureDevice() {
		this->stop();
		/* cleanup graph to allow resources to be freed */
		if (this->graph != NULL) {
			HRESULT res;
			ComPtr<IEnumFilters> filterEnum;
			ComPtr<IBaseFilter> filter;
			res = this->graph->EnumFilters(&filterEnum);
			if (res == S_OK) {
				while (filterEnum->Next(1, &filter, NULL) == S_OK) {
					this->graph->RemoveFilter(filter);
					filterEnum->Reset();
					filter.Reset();
				}
			}
		}
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
			if (this->devicePath != NULL) free(this->devicePath);
			if (this->deviceName != NULL) free(this->deviceName);
			this->Base::operator= (o);
			this->moniker = o.moniker;
			this->graph = o.graph;
			this->capture = o.capture;
			this->control = o.control;
			this->referenceClock = o.referenceClock;
			this->sourceFilter = o.sourceFilter;
			this->renderer = o.renderer;
			this->attachedSourceFilter = o.attachedSourceFilter;
			this->attachedRenderer = o.attachedRenderer;
			this->createdStream = o.createdStream;
			this->configuredSourceOutput = o.configuredSourceOutput;
			this->attachedControl = o.attachedControl;
			this->devicePath = NULL;
			this->deviceName = NULL;
			if ( this->moniker ) {
				/* ensure that the variables are set in case of device removal */
				this->getName();
				this->getPath();
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
	 *
	 * @remarks Creates and starts the DirectShow filter graph:
	 * Source Filter -> VKVM Capture Sink
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
		if (this->renderer != NULL) {
			this->renderer->setCallback(cb);
		}

		/* create the `FilterGraph` */
		if (this->graph == NULL) {
			res = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IFilterGraph2, &(this->graph));
			if (res != S_OK) return false;
		}

		/* create the `CaptureGraphBuilder` */
		if (this->capture == NULL) {
			res = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, &(this->capture));
			if (res != S_OK) return false;
			this->capture->SetFiltergraph(this->graph);
		}

		/* create the reference time from system time and use it */
		if (this->referenceClock == NULL) {
			res = CoCreateInstance(CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, IID_IReferenceClock, &(this->referenceClock));
			if (res != S_OK) return false;
		}

		/* attach the source filter */
		if ( ! this->attachedSourceFilter ) {
			res = this->graph->AddSourceFilterForMoniker(this->moniker, NULL, L"Source Filter", &(this->sourceFilter));
			if (res != S_OK) return false;
			res = this->sourceFilter->SetSyncSource(this->referenceClock);
			if (res != S_OK) return false;
			this->attachedSourceFilter = true;
		}

		/* get the controller for the graph */
		if (this->control == NULL) {
			res = this->graph->QueryInterface(IID_IMediaControl, &(this->control));
			if (res != S_OK) return false;
		}

		/* create the renderer */
		if (this->renderer == NULL) {
			this->renderer.Attach(new CaptureSink(*(this->Base::callback)));
			if (this->renderer == NULL) return false;
		}

		/* attach the renderer */
		if ( ! this->attachedRenderer ) {
			res = this->graph->AddFilter(this->renderer, FILTER_NAME);
			if (res != S_OK) return false;
			this->attachedRenderer = true;
		}

		/* configure video capture output format */
		if ( ! this->configuredSourceOutput ) {
			/* This step needs to be performed before the graph builder constructs the graph
			 * via `RenderStream` to allow it to handle resolution, compression and color space
			 * conversion correctly automatically. */
			res = this->configureSourceOutput(wnd);
			if (res != S_OK && res != S_FALSE) return false;
			this->configuredSourceOutput = true;
		}

		/* create filter stream */
		if ( ! this->createdStream ) {
			res = this->capture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, this->sourceFilter, NULL, this->renderer);
			if (res != S_OK) return false;
			this->createdStream = true;
		}

		/* start streaming */
		if ( ! this->attachedControl ) {
			res = this->capture->ControlStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, this->sourceFilter, &start, &stop, CTRL_STARTED, CTRL_STOPPED);
			if (res != S_OK && res != S_FALSE) return false;
			this->attachedControl = true;
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
		ComPtr<IPropertyBag> propBag;
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
		ComPtr<ISpecifyPropertyPages> propPages;
		ComPtr<IUnknown> filterUnk;
		FILTER_INFO filterInfo;
		bool hasFilterInfo = false;
		CAUUID caGUID;
		bool hasCaGUID = false;
		const auto cleanupOnReturn = makeScopeExit([&]() {
			if (hasFilterInfo && filterInfo.pGraph != NULL) filterInfo.pGraph->Release();
			if (hasCaGUID && caGUID.pElems != NULL) CoTaskMemFree(caGUID.pElems);
		});
		HRESULT res = this->sourceFilter.As(&propPages);
		if ( FAILED(res) ) return;
		res = this->sourceFilter->QueryFilterInfo(&filterInfo);
		if ( FAILED(res) ) return;
		hasFilterInfo = true;
		res = this->sourceFilter.As(&filterUnk);
		if ( FAILED(res) ) return;
		propPages->GetPages(&caGUID);
		hasCaGUID = true;
		IUnknown * objects[1] = { filterUnk };
		OleCreatePropertyFrame(
			parentWnd,           /* Parent window */
			0, 0,                /* Reserved */
			filterInfo.achName,  /* Caption for the dialog box */
			1,                   /* Number of objects (just the filter) */
			objects,             /* Array of object pointers. */
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
		ComPtr<IEnumPins> pinEnum;
		ComPtr<IPin> pin;
		PIN_INFO pinInfo;
		bool hasPinInfo = false;
		ComPtr<ISpecifyPropertyPages> propPages;
		ComPtr<IUnknown> filterUnk;
		CAUUID caGUID;
		bool hasCaGUID = false;
		HRESULT res = S_OK;
		const auto cleanupOnReturn = makeScopeExit([&]() {
			if (hasPinInfo && pinInfo.pFilter != NULL) pinInfo.pFilter->Release();
			if (hasCaGUID && caGUID.pElems != NULL) CoTaskMemFree(caGUID.pElems);
		});
		res = this->sourceFilter->EnumPins(&pinEnum);
		if ( FAILED(res) ) return res;
		/* open the configuration window for the first output pin found */
		while (pinEnum->Next(1, &pin, NULL) == S_OK) {
			const auto cleanupOnNext1 = makeScopeExit([&]() {
				pin.Reset();
			});
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
			res = pin.As(&propPages);
			if ( FAILED(res) ) return res;
			res = pin.As(&filterUnk);
			if ( FAILED(res) ) return res;
			propPages->GetPages(&caGUID);
			hasCaGUID = true;
			IUnknown * objects[1] = { filterUnk };
			return OleCreatePropertyFrame(
				parentWnd,       /* Parent window (this makes the new window modal) */
				0, 0,            /* Reserved */
				pinInfo.achName, /* Caption for the dialog box */
				1,               /* Number of objects (just the filter) */
				objects,         /* Array of object pointers. */
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
	 * @return 1 on success, 0 if not running and -1 on error
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
	ComPtr<ICreateDevEnum> devEnum;
	ComPtr<IEnumMoniker> monikerEnum;
	ComPtr<IMoniker> moniker;
	CaptureDeviceList result;

	/* instantiate enumerator for video input devices */
	res = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, &devEnum);
	if ( FAILED(res) ) return result;

	res = devEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &monikerEnum, 0);
	if (FAILED(res) || res == S_FALSE) return result;

	/* get video input device list items */
	while (monikerEnum->Next(1, &moniker, NULL) == S_OK) {
		result.push_back(new NativeCaptureDevice(moniker));
		moniker.Reset();
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
