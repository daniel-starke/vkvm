/**
 * @file Vkvm.cpp
 * @author Daniel Starke
 * @date 2019-10-11
 * @version 2024-02-18
 */
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>
#include <vkm-periphery/Framing.hpp>
#include <vkm-periphery/Protocol.hpp>
#include <libpcf/serial.h>
#include <libpcf/target.h>
#include <pcf/serial/Vkvm.hpp>
#include <pcf/ScopeExit.hpp>


#if defined(PCF_IS_WIN)
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef MB_RIGHT
#undef S_OK
#else /* !PCF_IS_WIN */
#include <cstdio>
extern "C" {
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <linux/input.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
}
#include <limits>
#include <pcf/UtilityLinux.hpp>
#ifndef PCF_MAX_SYS_PATH
#define PCF_MAX_SYS_PATH 1024
#endif
#ifndef PCF_MAX_UEVENT_LINE
#define PCF_MAX_UEVENT_LINE (PCF_MAX_SYS_PATH + 8)
#endif
#define GRAB_OFF reinterpret_cast<void *>(0)
#define GRAB_ON reinterpret_cast<void *>(1)
#endif /* !PCF_IS_WIN */


/** Receive buffer size in bytes. */
#define RECV_BUFFER_SIZE 1024
/** Send buffer size in bytes. */
#define SEND_BUFFER_SIZE 1024
/** Maximum number of outstanding requests. */
#define REQUEST_FIFO_LIMIT 128


/**
 * Returns the number of milliseconds passed since the program was started.
 *
 * @return milliseconds since program start
 */
extern "C" inline unsigned long millis(void) {
#ifdef PCF_IS_WIN
	static LARGE_INTEGER perfFreq;
	static int hasPerfFreq = 0;
	LARGE_INTEGER counter;
	if (hasPerfFreq == 0) {
		QueryPerformanceFrequency(&perfFreq);
		hasPerfFreq = 1;
	}
	QueryPerformanceCounter(&counter);
	return static_cast<unsigned long>(int64_t(counter.QuadPart) * INT64_C(1000) / int64_t(perfFreq.QuadPart));
#elif defined(PCF_IS_LINUX)
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) return 0;
	return static_cast<unsigned long>(((int64_t(tv.tv_sec) * INT64_C(1000000)) + int64_t(tv.tv_usec)) / UINT64_C(1000));
#else /* not PCF_IS_WIN and not PCF_IS_LINUX */
#error Unsupported target OS.
#endif
}


#ifdef VKVM_TRACE
#include <stdarg.h>
#include <stdio.h>
/** Helper structure for VKVM serial tracing. */
struct VkvmTrace {
	FILE * fd; /**< File descriptor of the trace file. */
	std::mutex mutex; /**< Guard for the I/O operations. */
	/** Default constructor. */
	explicit inline VkvmTrace(): fd(NULL) {}
	/* Destructor. */
	inline ~VkvmTrace() {
		if (this->fd != NULL) fclose(this->fd);
	}
};

/**
 * VKVM trace function. All output is written to "vkvm.log".
 *
 * @param[in] lock - 0 no lock, 1 begin lock, 2 end lock, 3 begin and end lock (single locked trace)
 * @param[in] fmt - `printf()` like format string
 * @param[in] ... - fmt specific parameters
 */
static inline void vkvmTrace(const int lock, const char * fmt, ...) PCF_PRINTF(2, 3);
static inline void vkvmTrace(const int lock, const char * fmt, ...) {
	static VkvmTrace log;
	if ((lock & 1) != 0) log.mutex.lock();
	if (log.fd == NULL) log.fd = fopen("vkvm.log", "a");
	if (log.fd == NULL) {
		if ((lock & 2) != 0) log.mutex.unlock();
		return;
	}
	if ((lock & 1) != 0) fprintf(log.fd, "%lu\t", millis());
	va_list args;
	va_start(args, fmt);
	vfprintf(log.fd, fmt, args);
	va_end(args);
	if ((lock & 2) != 0) {
		fflush(log.fd);
		log.mutex.unlock();
	}
}
#else /* !VKVM_TRACE */
#define vkvmTrace(lock, ...) {}
#endif /* VKVM_TRACE */


namespace pcf {
namespace serial {
namespace {


/* forward declaration */
struct SerialCommon;


/**
 * Reference counting byte buffer.
 *
 * @remarks This class is not thread-safe.
 */
class ByteBuffer {
private:
	/**
	 * Internal structure which holds the actual data.
	 * The actual size depends on the byte buffer size.
	 */
	struct Rc {
		std::atomic_size_t refCount; /**< Current reference count. The object is deleted if this reaches zero. */
		uint8_t size; /**< Byte buffer size. */
		uint8_t buffer[1]; /**< Pointer to the first byte of the byte buffer. */
	};
	Rc * ptr; /**< Pointer to the byte buffer object. */
public:
	/**
	 * Constructor.
	 *
	 * @param[in] buf - initial buffer data to use
	 * @param[in] len - buffer size
	 */
	explicit inline ByteBuffer(const uint8_t * buf, const uint8_t len) {
		this->ptr = static_cast<Rc *>(malloc(offsetof(Rc, buffer) + size_t(len)));
		if (this->ptr == NULL) throw std::bad_alloc();
		this->ptr->refCount.store(1, std::memory_order_relaxed);
		this->ptr->size = len;
		memcpy(this->ptr->buffer, buf, size_t(len));
	}

	/**
	 * Copy constructor.
	 *
	 * @param[in] o - object to copy
	 * @remarks This only increases the reference counter.
	 */
	inline ByteBuffer(const ByteBuffer & o):
		ptr(o.ptr)
	{
		this->aquire();
	}

	/**
	 * Move constructor.
	 *
	 * @param[in,out] o - object to move
	 * @remarks The object moved is no longer available from its original identifier after this.
	 */
	inline ByteBuffer(ByteBuffer && o):
		ptr(o.ptr)
	{
		o.ptr = NULL;
	}

	/**
	 * Destructor.
	 */
	inline ~ByteBuffer() {
		this->release();
	}

	/**
	 * Assignment operator.
	 *
	 * @param[in] o - object o assign
	 * @remarks This only modifies the reference counter.
	 */
	inline ByteBuffer & operator= (const ByteBuffer & o) {
		if (&o == this) return *this;
		this->release();
		this->ptr = o.ptr;
		this->aquire();
		return *this;
	}

	/**
	 * Move assignment operator.
	 *
	 * @param[in,out] o - object to move
	 * @remarks The object moved is no longer available from its original identifier after this.
	 */
	inline ByteBuffer & operator= (ByteBuffer && o) {
		this->release();
		this->ptr = o.ptr;
		o.ptr = NULL;
		return *this;
	}

	/**
	 * Returns the raw pointer to the internal byte buffer.
	 *
	 * @return byte buffer pointer
	 */
	inline uint8_t * getPointer() {
		return (this->ptr != NULL) ? this->ptr->buffer : NULL;
	}

	/**
	 * Returns the raw pointer to the internal byte buffer.
	 *
	 * @return byte buffer pointer
	 */
	inline const uint8_t * getPointer() const {
		return (this->ptr != NULL) ? this->ptr->buffer : NULL;
	}

	/**
	 * Returns the internal byte buffer size.
	 *
	 * @return byte buffer size
	 */
	inline uint8_t getSize() const {
		return (this->ptr != NULL) ? this->ptr->size : 0;
	}
private:
	/**
	 * Increases the reference counter.
	 */
	inline void aquire() {
		if (this->ptr == NULL) return;
		this->ptr->refCount.fetch_add(1, std::memory_order_relaxed);
	}

	/**
	 * Decreases the reference counter and deletes the pointed
	 * object when it reaches zero.
	 */
	inline void release() {
		if (this->ptr == NULL) return;
		if (this->ptr->refCount.fetch_sub(1, std::memory_order_release) == 1) {
			std::atomic_thread_fence(std::memory_order_acquire);
			free(this->ptr);
			this->ptr = NULL;
		}
	}
};


/** Expands a single parameter to a `parameter_pack`. */
template <typename T> struct ExpandArg { typedef parameter_pack<T> type; };
/** Specialization for `ByteBuffer` which expands to the raw pointer and data length. */
template <> struct ExpandArg<ByteBuffer> { typedef parameter_pack<const uint8_t *, uint8_t> type; };
/** Specialization for void which expands to an empty `parameter_pack`. */
template <> struct ExpandArg<void> { typedef parameter_pack<> type; };

/** Unpacks a `parameter_pack` to the argument list of a given method pointer type. */
template <typename ...> struct UnpackToCallback;
template <typename R, typename C, typename ...Args>
struct UnpackToCallback<R, C, parameter_pack<Args...>> {
	typedef R(C::* type)(const VkvmCallback::PeripheryResult, Args...);
};


/** Maps a request parameter type to itself. */
template <typename From, typename To>
inline To mapRequestParam(const From & from) { return from; }
/** Specialization to map a `ByteBuffer` to its stored data pointer. */
template <>
inline const uint8_t * mapRequestParam(const ByteBuffer & from) { return static_cast<const uint8_t *>(from.getPointer()); }
/** Specialization to map a `ByteBuffer` to its stored data length. */
template <>
inline uint8_t mapRequestParam(const ByteBuffer & from) { return from.getSize(); }


/**
 * General request item interface. This is non-copyable.
 */
struct RequestQueueItem {
	RequestQueueItem * next; /**< Pointer to the next request item in the queue. */

	const uint8_t seq; /**< Associated frame sequence number. */
	const RequestType::Type type; /**< Associated request type. See vkm-periphery/Protocol.hpp */

	/**
	 * Constructor.
	 *
	 * @param[in] s - frame sequence number to use
	 * @param[in] t - request type to use (see vkm-periphery/Protocol.hpp)
	 */
	explicit inline RequestQueueItem(const uint8_t s, const RequestType::Type t):
		next(NULL),
		seq(s),
		type(t)
	{}

	/** Destructor. */
	virtual ~RequestQueueItem() {}

	/** Deleted copy constructor. */
	RequestQueueItem(const RequestQueueItem &) = delete;
	/** Deleted assignment operator. */
	RequestQueueItem & operator= (const RequestQueueItem &) = delete;

	/**
	 * Send out the stored request to the serial device given.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments reference
	 * @return true on success, else false
	 */
	virtual bool send(SerialCommon &) const = 0;
	/**
	 * Called to set the result value from the given data.
	 *
	 * @param[in] buf - input data buffer
	 * @param[in] len - input data length
	 * @return true on success, else false
	 */
	virtual bool setResult(const uint8_t *, const size_t) = 0;
	/**
	 * Called to report the result to the registered function.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments reference
	 * @param[in] res - result code for the request
	 */
	virtual void report(SerialCommon &, const VkvmCallback::PeripheryResult) const = 0;
};


/**
 * Object with the shared `VkvmDevice` arguments.
 */
struct SerialCommon {
	std::mutex disconnectMutex; /**< Guard to avoid parallel disconnect operations. */
	std::mutex openCloseMutex; /**< Guard to avoid serial device open/close operations. */
	std::mutex queueMutex; /**< Guard to sequentialize request queuing. */
	std::mutex readMutex; /**< Locked until serial read thread termination. */
	std::mutex writeMutex; /**< Locked until serial write thread termination. */
	std::thread disconnectThread; /**< Thread for asynchronous serial device disconnect operations. */
	std::condition_variable writable; /**< To wake up the serial write thread. */
	VkvmDevice * volatile device; /**< Reference to the owning `VkvmDevice` instance. */
	Framing<VKVM_MAX_FRAME_SIZE> * volatile framing; /**< Serial device framing handler. This is used for input and output operations. */
	uint8_t buffer[SEND_BUFFER_SIZE]; /**< Serial send buffer. */
	size_t bufferSize; /**< Current data size within the serial send buffer. */
	RequestQueueItem * reqFifoFirst; /**< Pointer to the first element of the request queue. */
	RequestQueueItem * reqFifoLast; /**< Pointer to the last element of the request queue. */
	size_t reqFifoSize; /**< Number of pending requests in the queue. */
	uint8_t reqNumber; /**< Next request frame sequence number. Note that zero is reserved for interrupts messages. */
	bool reqPending; /**< True if there is an outstanding request for which the result has not yet been received. */
	size_t tickDuration; /**< Interval in milliseconds at which the read thread checks related events (e.g. disconnect request). */
	size_t timeout; /**< Serial device open/write/request response timeout in milliseconds. */
	unsigned long lastSent; /**< Timestampt (derived from millis()) at which the last request has been set. */
	VkvmCallback * volatile callback; /**< Reference to a callback handler used for device changes, unsolicited responses and request responses. */
	tSerial * volatile serial; /**< Low level serial device handler. */
	volatile bool connected; /**< Set if there is an open serial connection to the VKVM periphery. */
	volatile bool terminate; /**< Set if the serial connection to the VKVM periphery has been terminated. Checked by the read/write threads. */
	uint8_t lastUsbState; /**< Most recently received USB periphery state. */
	uint8_t lastLEDs; /**< Most recently received keyboard status LED bits. */
	/**< Possible hook procedure states. */
	enum HookProcState {
		HPS_START, /**< The keyboard/mouse hook has been installed. */
		HPS_IDLE, /**< The keyboard/mouse hook is waiting for events. */
		HPS_FAILED /**< Failed installing the keyboard/mouse hook. */
	};
	volatile HookProcState hookProcState; /**< Current keyboard/mouse hook status. */
	std::mutex hookProcWait; /**< Used to wait for the keyboard/mouse hook registration thread to finish. */
	std::thread hookProcThread; /**< Background thread which registers the hook and to processed keyboard/mouse events. */
#if defined(PCF_IS_WIN)
	long lastMouseX; /**< Most recent X position of the grabbed mouse. */
	long lastMouseY; /**< Most recent Y position of the grabbed mouse. */
	bool hasLastMouse; /**< Set if `lastMouseX` and `lastMouseY` is valid. */
	DWORD hookProcThreadId; /**< ID of hookProcThread to request termination of that thread. */
	HHOOK keyboardHook; /**< Handle of the keyboard hook. */
	HHOOK mouseHook; /**< Handle of the mouse hook. */
#elif defined(PCF_IS_LINUX)
	int hookTermFd; /**< File descriptor to transmit termination request events. */
#endif /* PCF_IS_LINUX */
};


/**
 * Reads the big-endian encoded request result value of the given type.
 *
 * @param[in] buf - input data buffer
 * @param[in] len - input data length
 * @param[out] out - variable which receives the result
 * @return true on success, else false
 * @tparam T - output variable type
 */
template <typename T, typename enable_if<is_arithmetic<T>::value>::type * = nullptr>
inline bool readBigEndian(const uint8_t * buf, const size_t len, T & out) {
	if (buf == NULL || sizeof(T) > len) return false;
	if (sizeof(T) > 1) {
#if __FRAMING_HPP__IS_BIG_ENDIAN
		memcpy(reinterpret_cast<uint8_t *>(&out), buf, sizeof(T));
#else /* reverse big endian to little endian */
		uint8_t * ptr = reinterpret_cast<uint8_t *>(&out) + sizeof(T) - 1;
		for (size_t i = 0; i < sizeof(T); i++) {
			*ptr-- = buf[i];
		}
#endif
	} else {
		out = *buf;
	}
	return true;
}

/**
 * Specialized request item instance. This is non-copyable.
 */
template <typename R, typename ...Args>
struct RequestQueueItemT : public RequestQueueItem {
	/** Used to map result type `void` to nothing. */
	struct Empty {};
	/** Callback type derived from the method of `VkvmCallback` matching the request parameter types for unambiguous resolution. */
	typedef typename UnpackToCallback<void, VkvmCallback, typename mapped_parameter_pack<ExpandArg, R, Args...>::type>::type Callback;
	/** Tuple type for the mapped request result report arguments. E.g. ByteBuffer is expanded to `<const uint8_t *, uint8_t>`. */
	typedef typename tuple_from<typename mapped_parameter_pack<ExpandArg, Args...>::type>::type ReportParams;
	/** Mapped result type. Nothing if void or the actual type. */
	typedef typename conditional<is_void<R>::value, Empty, tuple<R> >::type Result;
	/**< Tupe type of the request parameters to store them. */
	typedef tuple<Args...> Params;

	const Callback callback; /**< Handle to the associated callback function. */
	const Params params; /**< Request parameters. */
	Result result; /**< Result value (if any). */

	/**
	 * Constructor.
	 *
	 * @param[in] s - frame sequence number to use
	 * @param[in] t - request type
	 * @param[in] cb - function to call on completion (success and error)
	 * @param[in] p - request parameters
	 */
	explicit inline RequestQueueItemT(const uint8_t s, const RequestType::Type t, const Callback cb, Args... p):
		RequestQueueItem(s, t),
		callback(cb),
		params(forward<Args>(p)...)
	{}

	/**
	 * Destructor.
	 */
	virtual ~RequestQueueItemT() {}

	/**
	 * Serialize the request and write it to the serial VKVM periphery interface.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments
	 * @return true on success, else false
	 */
	virtual bool send(SerialCommon & args) const override {
		if (args.framing == NULL || args.terminate) return false;
		/* first set sent time and then pending state to ensure timeout checks work properly */
		args.lastSent = millis();
		args.reqPending = true;
		if ( ! args.framing->beginTransmission(this->seq) ) return false;
		if ( ! args.framing->write(uint8_t(this->type)) ) return false;
		if ( ! this->sendParams(args, typename make_index_sequence<sizeof...(Args)>::type()) ) return false;
		if ( ! args.framing->endTransmission() ) return false;
		return true;
	}

	/**
	 * Sets the request result from the given received data buffer.
	 *
	 * @param[in] buf - buffer with the received request result
	 * @param[in] len - buffer length
	 * @return true on success, else false
	 */
	virtual bool setResult(const uint8_t * buf, const size_t len) override {
		if (buf == NULL) return false;
		return this->extractRemoteCallResult<R>(buf, len);
	}

	/**
	 * Calls the associated callback function with the request result code,
	 * request response (if required) and the associated parameters of request.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments
	 * @param[in] res - request result code
	 */
	virtual void report(SerialCommon & args, const VkvmCallback::PeripheryResult res) const override {
		if (args.callback == NULL || this->callback == NULL) return;
		this->reportInternal<R>(args, res, typename ParamIndexGen<Args...>::type(), typename make_index_sequence<tuple_size<ReportParams>::value>::type());
	}
private:
	/**
	 * Request parameter indices for parameter iteration.
	 *
	 * @tparam i - this index
	 * @tparam N - number of remaining indices
	 * @tparam S - added indices
	 * @tparam Ts - remaining parameter types
	 */
	template <size_t, size_t, typename, typename> struct ParamIndices;

	/**
	 * Request parameter indices for parameter iteration.
	 *
	 * @tparam i - this index
	 * @tparam N - number of remaining indices
	 * @tparam S - added indices
	 * @tparam Ts - remaining parameter types
	 */
	template <size_t i, size_t N, size_t ...S, typename ...Ts> struct ParamIndices<i, N, index_sequence<S...>, parameter_pack<Ts...>> : ParamIndices<i + 1, N - 1, index_sequence<S..., i>, parameter_pack<Ts...>> {};

	/**
	 * Request parameter indices for parameter iteration.
	 *
	 * @tparam i - this index
	 * @tparam N - number of remaining indices
	 * @tparam T0 - this parameter type
	 * @tparam S - added indices
	 * @tparam Ts - remaining parameter types
	 */
	template <size_t i, size_t N, typename T0, size_t ...S, typename ...Ts> struct ParamIndices<i, N, index_sequence<S...>, parameter_pack<T0, Ts...>> : ParamIndices<i + 1, N - 1, index_sequence<S..., i>, parameter_pack<Ts...>> {};

	/**
	 * Request parameter indices for parameter iteration.
	 * This is the specialization for `ByteBuffer` which expands to `<const uint8_t * buf, uint8_t len>`.
	 *
	 * @tparam i - this index
	 * @tparam N - number of remaining indices
	 * @tparam S - added indices
	 * @tparam Ts - remaining parameter types
	 */
	template <size_t i, size_t N, size_t ...S, typename ...Ts> struct ParamIndices<i, N, index_sequence<S...>, parameter_pack<ByteBuffer, Ts...>> : ParamIndices<i + 1, N - 1, index_sequence<S..., i, i>, parameter_pack<Ts...>> {};

	/**
	 * Request parameter indices for parameter iteration.
	 * Initial case.
	 *
	 * @tparam i - first index
	 * @tparam S - indices
	 * @tparam Ts - parameter types
	 */
	template <size_t i, size_t ...S, typename ...Ts> struct ParamIndices<i, 0, index_sequence<S...>, parameter_pack<Ts...>> { typedef index_sequence<S...> type; };

	/**
	 * Request parameter indices generator for parameter iteration.
	 *
	 * @tparam Ts - parameter types
	 */
	template <typename ...Ts> struct ParamIndexGen : ParamIndices<0, sizeof...(Ts), index_sequence<>, parameter_pack<Ts...>> {};

	/**
	 * Extracts a single result value. The value is stored in `result`.
	 * The value is stored big endian encoded in the given buffer.
	 *
	 * @param[in] buf - input buffer
	 * @param[in] len - input length
	 * @return true on success, else false
	 * @tparam T - value type (void case)
	 */
	template <typename T, typename enable_if<is_void<T>::value>::type * = nullptr>
	inline bool extractRemoteCallResult(const uint8_t * buf, const size_t len) {
		return true;
	}

	/**
	 * Extracts a single result value. The value is stored in `result`.
	 * The value is stored big endian encoded in the given buffer.
	 *
	 * @param[in] buf - input buffer
	 * @param[in] len - input length
	 * @return true on success, else false
	 * @tparam T - value type (non-void case)
	 */
	template <typename T, typename enable_if<!is_void<T>::value>::type * = nullptr>
	inline bool extractRemoteCallResult(const uint8_t * buf, const size_t len) {
		return readBigEndian(buf, len, get<0>(this->result));
	}

	/**
	 * Calls the stored callback method with the request response and initial
	 * request parameters.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments
	 * @param[in] res - request result code
	 * @param[in] fromSeq - type only index sequence of the initial request parameters
	 * @param[in] toSeq - type only index sequence of the callback parameters
	 * @tparam T - result type (void case)
	 * @tparam Fi - parameter indices
	 * @tparam Ti - parameter value types
	 */
	template <typename T, size_t ...Fi, size_t ...Ti, typename enable_if<is_void<T>::value>::type * = nullptr>
	inline void reportInternal(SerialCommon & args, const VkvmCallback::PeripheryResult res, const index_sequence<Fi...>, const index_sequence<Ti...>) const {
		(args.callback->*(this->callback))(
			res,
			this->getParam<Fi, typename tuple_element<Fi, Params>::type, typename tuple_element<Ti, ReportParams>::type>()...
		);
	}

	/**
	 * Calls the stored callback method with the request response and initial
	 * request parameters.
	 *
	 * @param[in,out] args - shared `VkvmDevice` arguments
	 * @param[in] res - request result code
	 * @param[in] fromSeq - type only index sequence of the initial request parameters
	 * @param[in] toSeq - type only index sequence of the callback parameters
	 * @tparam T - result type (non-void case)
	 */
	template <typename T, size_t ...Fi, size_t ...Ti, typename enable_if<!is_void<T>::value>::type * = nullptr>
	inline void reportInternal(SerialCommon & args, const VkvmCallback::PeripheryResult res, const index_sequence<Fi...>, const index_sequence<Ti...>) const {
		(args.callback->*(this->callback))(
			res,
			get<0>(this->result),
			this->getParam<Fi, typename tuple_element<Fi, Params>::type, typename tuple_element<Ti, ReportParams>::type>()...
		);
	}

	/**
	 * Returns a single initial request parameter as callback parameter type.
	 *
	 * @return mapped callback parameter
	 * @tparam i - initial request parameter tuple index
	 * @tparam From - initial request parameter type
	 * @tparam To - callback parameter type
	 */
	template <size_t i, typename From, typename To>
	inline To getParam() const {
		return mapRequestParam<From, To>(get<i>(this->params));
	}

	/**
	 * Serializes a single request parameter on the serial VKVM interface
	 * within the current frame.
	 *
	 * @param[in] args - shared `VkvmDevice` arguments
	 * @param[in] param - request parameter
	 * @return true on success, else false
	 * @tparam T - request parameter type
	 */
	template <typename T>
	inline bool sendParam(SerialCommon & args, const T param) const {
		return args.framing->write(param);
	}

	/**
	 * Serializes a single request parameter on the serial VKVM interface
	 * within the current frame.
	 * This is the specialization for the `ByteBuffer` type.
	 *
	 * @param[in] args - shared `VkvmDevice` arguments
	 * @param[in] param - request parameter
	 * @return true on success, else false
	 */
	inline bool sendParam(SerialCommon & args, const ByteBuffer & param) const {
		return args.framing->write(static_cast<const uint8_t *>(param.getPointer()), size_t(param.getSize()));
	}

	/**
	 * Serializes all request parameters on the serial VKVM interface
	 * within the current frame.
	 * Final case.
	 *
	 * @param[in] args - shared `VkvmDevice` arguments
	 * @return true on success, else false
	 */
	inline bool sendParamsUnpacked(SerialCommon & args) const {
		return true;
	}

	/**
	 * Serializes all request parameters on the serial VKVM interface
	 * within the current frame.
	 *
	 * @param[in] args - shared `VkvmDevice` arguments
	 * @param[in] p0 - current parameter to send
	 * @param[in] rest - remaining parameters to send
	 * @return true on success, else false
	 * @tparam T0 - current parameter type
	 * @tparam Rest - remaining parameter types
	 */
	template <typename T0, typename ...Rest>
	inline bool sendParamsUnpacked(SerialCommon & args, const T0 & p0, Rest... rest) const {
		if ( ! this->sendParam(args, p0) ) return false;
		return this->sendParamsUnpacked(args, rest...);
	}

	/**
	 * Serializes all request parameters on the serial VKVM interface
	 * within the current frame.
	 *
	 * @param[in] args - shared `VkvmDevice` arguments
	 * @param[in] seq - type only index sequence of the tuple parameter values
	 * @return true on success, else false
	 * @tparam i - tuple indices
	 */
	template <size_t ...i>
	inline bool sendParams(SerialCommon & args, const index_sequence<i...>) const {
		return this->sendParamsUnpacked(args, get<i>(this->params)...);
	}
};


/**
 * Queues a request which shall be sent to the connected VKVM periphery.
 * The request consists of the request type, a callback method called upon reception of
 * the result from the periphery and the arguments according to the request type.
 *
 * @param[in] args - object with the shared `VkvmDevice` arguments
 * @param[in] type - VKVM protocol specific request type
 * @param[in] callback - `VkvmCallback` method which receives the request result
 * @param[in] params - request specific parameters
 * @return true on success, else false
 * @see vkm-periphery/Protocol.hpp
 */
template <typename R, typename ...Args>
bool serialQueueCommand(SerialCommon & args, const RequestType::Type type, typename RequestQueueItemT<R, Args...>::Callback callback, Args... params) {
	if (args.device == NULL || args.terminate || args.reqFifoSize >= REQUEST_FIFO_LIMIT) return false;
	std::unique_lock<std::mutex> guard(args.queueMutex);
	if (args.reqNumber == 0) args.reqNumber++; /* zero is reserved for interrupts messages */
	const uint8_t seq = args.reqNumber++;
	RequestQueueItem * item = new RequestQueueItemT<R, Args...>(seq, type, callback, params...);
	if (args.reqFifoFirst == NULL) {
		args.reqFifoFirst = item;
	} else {
		args.reqFifoLast->next = item;
	}
	args.reqFifoLast = item;
	args.reqFifoSize++;
	guard.unlock();
	args.writable.notify_one();
	return true;
}


/**
 * Performs an asynchronous disconnect of the serial connected VKVM periphery.
 *
 * @param[in] args - object with the shared `VkvmDevice` arguments
 * @param[in] reason - disconnect reason forwarded to the registered callback
 */
static void serialDisconnect(SerialCommon & args, VkvmCallback::DisconnectReason reason) {
	vkvmTrace(3, "disconnect\t%i\n", int(reason));
	if ( ! args.disconnectMutex.try_lock() ) return; /* only one instance shall call disconnect */
	vkvmTrace(3, "disconnect forwarded\t%i\n", int(reason));
	if ( args.disconnectThread.joinable() ) args.disconnectThread.join();
	args.disconnectThread = std::thread([&, reason] () mutable {
		try {
			VkvmCallback * cb = NULL;
			std::lock_guard<std::mutex> dcGuard(args.disconnectMutex, std::adopt_lock);
			if ( ! args.openCloseMutex.try_lock() ) {
				std::lock_guard<std::mutex> queueGuard(args.queueMutex);
				if ( args.terminate ) {
					if (args.connected && args.callback != NULL) {
						cb = args.callback;
						args.callback = NULL;
						cb->onVkvmDisconnected(VkvmCallback::DisconnectReason::D_USER);
					}
					args.connected = false;
					return; /* mutex is locked -> return to VkvmDevice::open() or VkvmDevice::close() */
				} else {
					args.openCloseMutex.lock(); /* this line should never be reached */
				}
			}
			vkvmTrace(3, "close2\t0x%p\n", static_cast<const void *>(args.serial));
			std::lock_guard<std::mutex> openCloseGuard(args.openCloseMutex, std::adopt_lock);
			args.terminate = true; /* signal remaining read/write threads to terminate */
			args.writable.notify_one();
			std::lock_guard<std::mutex> readGuard(args.readMutex); /* wait for read thread to terminate */
			std::lock_guard<std::mutex> writeGuard(args.writeMutex); /* wait for write thread to terminate */
			cb = args.callback;
			if (args.serial == NULL) return;
			args.callback = NULL;
			const void * oldHandle = args.serial;
			PCF_UNUSED(oldHandle);
			ser_delete(args.serial);
			args.serial = NULL;
			args.connected = false;
			args.terminate = false;
			args.reqPending = false;
			vkvmTrace(3, "closed2\t0x%p\n", static_cast<const void *>(args.serial));
			if (cb != NULL) cb->onVkvmDisconnected(reason);
		} catch (...) {}
	});
}


/**
 * Writes data to the serial connected VKVM periphery. This function is being
 * used as callback for the `Framing` class. Data is first written to an internal
 * buffer and actually send to the periphery at each complete frame or when
 * reaching the end of the internal buffer size.
 *
 * @param[in] argsPtr - object with the shared `VkvmDevice` arguments
 * @param[in] val - single byte value to write
 * @param[in] eof - true to force data transmission (end of frame), else false
 * @return true on success, else false
 */
static bool serialWrite(void * argsPtr, const uint8_t val, const bool eof) {
	if (argsPtr == NULL) return false;
	SerialCommon * args = static_cast<SerialCommon *>(argsPtr);
	if (args->serial == NULL || args->terminate) return false;
	args->buffer[args->bufferSize++] = val;
	if (!eof && args->bufferSize < SEND_BUFFER_SIZE) return true;
#ifdef VKVM_TRACE
	vkvmTrace(1, "out\t");
	for (size_t i = 0; i < args->bufferSize; i++) {
		if (i != 0) vkvmTrace(0, " ");
		vkvmTrace(0, "%02X", unsigned(args->buffer[i] & 0xFF));
	}
	vkvmTrace(2, "\n");
#endif /* VKVM_TRACE */
	const ssize_t res = ser_write(args->serial, args->buffer, args->bufferSize, args->timeout);
	const bool success = (res == ssize_t(args->bufferSize));
	args->bufferSize = 0;
	return success;
}


/**
 * Read handler for data from the serial connected VKVM periphery. This function
 * is being used as callback for the `Framing` class. It handles outstanding request
 * responses and unsolicited responses.
 *
 * @param[in] args - object with the shared `VkvmDevice` arguments
 * @param[in] seq - frame sequence number
 * @param[in] buf - payload data buffer
 * @param[in] len - payload data length
 * @param[in] err - true if an error occurred parsing the frame, else false
 */
static void serialReadHandler(SerialCommon & args, const uint8_t seq, uint8_t * buf, const size_t len, const bool err) {
	if (args.device == NULL || args.framing == NULL || args.callback == NULL || args.serial == NULL || args.terminate || buf == NULL) {
		return;
	}
	if (len < 1 || err) {
		args.callback->onVkvmBrokenFrame();
		return;
	}
	RequestQueueItem * item = args.reqFifoFirst;
	if (item != NULL && size_t(millis() - args.lastSent) >= args.timeout) {
		vkvmTrace(3, "timeout1\t%lu\n", args.lastSent);
		serialDisconnect(args, VkvmCallback::DisconnectReason::D_TIMEOUT);
		return;
	}
	VkvmCallback::PeripheryResult res = VkvmCallback::PeripheryResult::COUNT;
	switch (buf[0]) {
	case ResponseType::S_OK:
		res = VkvmCallback::PeripheryResult::PR_OK;
		break;
	case ResponseType::E_BROKEN_FRAME:
		res = VkvmCallback::PeripheryResult::PR_BROKEN_FRAME;
		break;
	case ResponseType::E_UNSUPPORTED_REQ_TYPE:
		res = VkvmCallback::PeripheryResult::PR_UNSUPPORTED_REQ_TYPE;
		break;
	case ResponseType::E_INVALID_REQ_TYPE:
		res = VkvmCallback::PeripheryResult::PR_INVALID_REQ_TYPE;
		break;
	case ResponseType::E_INVALID_FIELD_VALUE:
		res = VkvmCallback::PeripheryResult::PR_INVALID_FIELD_VALUE;
		break;
	case ResponseType::E_HOST_WRITE_ERROR:
		res = VkvmCallback::PeripheryResult::PR_HOST_WRITE_ERROR;
		break;
	case ResponseType::I_USB_STATE_UPDATE:
		if (len == 2) {
			args.lastUsbState = buf[1];
			args.callback->onVkvmUsbState(VkvmCallback::PeripheryResult::PR_OK, buf[1]);
			return;
		}
		args.callback->onVkvmBrokenFrame();
		return;
	case ResponseType::I_LED_UPDATE:
		if (len == 2) {
			args.lastLEDs = buf[1];
			args.callback->onVkvmKeyboardLeds(VkvmCallback::PeripheryResult::PR_OK, buf[1]);
			return;
		}
		args.callback->onVkvmBrokenFrame();
		return;
	case ResponseType::D_MESSAGE:
		return;
	default:
		args.callback->onVkvmBrokenFrame();
		return;
	}
	if (item == NULL || item->seq != seq) {
		/* A response was received without any pending request. */
		vkvmTrace(3, "invalid\t%u\n", unsigned(seq));
		return;
	}
	switch (item->type) {
	case RequestType::GET_USB_STATE:
		if (res == VkvmCallback::PeripheryResult::PR_OK && len >= 2) {
			args.lastUsbState = buf[1];
		}
		break;
	case RequestType::GET_KEYBOARD_LEDS:
		if (res == VkvmCallback::PeripheryResult::PR_OK && len >= 2) {
			args.lastLEDs = buf[1];
		}
		break;
	default:
		break;
	}
	switch (item->type) {
	case RequestType::GET_PROTOCOL_VERSION:
		if (res != VkvmCallback::PeripheryResult::PR_OK || len < 3 || (uint16_t(buf[2]) | (uint16_t(buf[1]) << 8)) != VKVM_PROT_VERSION) {
			serialDisconnect(args, VkvmCallback::DisconnectReason::D_INVALID_PROTOCOL);
		} else {
			args.connected = true;
			args.callback->onVkvmConnected();
			/* cannot fail because the queue is still empty */
			serialQueueCommand<uint8_t>(args, RequestType::GET_USB_STATE, &VkvmCallback::onVkvmUsbState);
			serialQueueCommand<uint8_t>(args, RequestType::GET_KEYBOARD_LEDS, &VkvmCallback::onVkvmKeyboardLeds);
		}
		break;
	case RequestType::GET_ALIVE:
		/* no callback is triggered on success, only on error */
		break;
	case RequestType::GET_USB_STATE:
	case RequestType::GET_KEYBOARD_LEDS:
	case RequestType::SET_KEYBOARD_DOWN:
	case RequestType::SET_KEYBOARD_UP:
	case RequestType::SET_KEYBOARD_ALL_UP:
	case RequestType::SET_KEYBOARD_PUSH:
	case RequestType::SET_KEYBOARD_WRITE:
	case RequestType::SET_MOUSE_BUTTON_DOWN:
	case RequestType::SET_MOUSE_BUTTON_UP:
	case RequestType::SET_MOUSE_BUTTON_ALL_UP:
	case RequestType::SET_MOUSE_BUTTON_PUSH:
	case RequestType::SET_MOUSE_MOVE_ABS:
	case RequestType::SET_MOUSE_MOVE_REL:
	case RequestType::SET_MOUSE_SCROLL:
		if ( item->setResult(buf + 1, (len >= 1) ? size_t(len - 1) : 0) ) {
			item->report(args, res);
		} else {
			args.callback->onVkvmBrokenFrame();
		}
		break;
	default:
		args.callback->onVkvmBrokenFrame();
		break;
	}
	/* remove item from queue */
	std::unique_lock<std::mutex> guard(args.queueMutex);
	if (item->next == NULL) {
		args.reqFifoFirst = NULL;
		args.reqFifoLast = NULL;
	} else {
		args.reqFifoFirst = item->next;
	}
	args.reqFifoSize--;
	args.reqPending = false;
	guard.unlock();
	args.writable.notify_one();
	delete item;
}


/**
 * Background thread which reads data from the serial connected
 * VKVM periphery. The received data is being parsed, processed
 * and forwarded to the corresponding registered `VkvmCallback` methods.
 * This also handles pending request timeouts and keep alive
 * request generation.
 *
 * @param[in] args - object with the shared `VkvmDevice` arguments
 */
static void serialReadThread(SerialCommon & args) {
	try {
		std::lock_guard<std::mutex> readGuard(args.readMutex);
		if (args.device == NULL || args.framing == NULL || args.callback == NULL || args.serial == NULL || args.terminate) {
			if ( args.terminate ) {
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_USER);
			} else {
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_SEND_ERROR);
			}
			return;
		}
		uint8_t * recvBuffer = new uint8_t[RECV_BUFFER_SIZE];
		const auto freeRecvBufferOnReturn = makeScopeExit([&]() { delete [] recvBuffer; });
		/* initially send the protocol version request to check the version */
		if ( ! serialQueueCommand<void>(args, RequestType::GET_PROTOCOL_VERSION, NULL) ) {
			serialDisconnect(args, VkvmCallback::DisconnectReason::D_SEND_ERROR);
			return;
		}
		while ( ! args.terminate ) {
			/* read data and process incoming frames from the serial device */
			const ssize_t res = ser_read(args.serial, recvBuffer, RECV_BUFFER_SIZE, args.tickDuration);
			if (res == -1) {
				/* read error */
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_RECV_ERROR);
				return;
			}
#ifdef VKVM_TRACE
			if (res > 0) {
				/* trace output */
				vkvmTrace(1, "in\t");
				for (ssize_t i = 0; i < res; i++) {
					if (i != 0) vkvmTrace(0, " ");
					vkvmTrace(0, "%02X", unsigned(recvBuffer[i] & 0xFF));
				}
				vkvmTrace(2, "\n");
			}
#endif /* VKVM_TRACE */
			for (ssize_t i = 0; i < res; i++) {
				auto readHandlerMapping = [&](const uint8_t seq, uint8_t * buf, const size_t len, const bool err) -> bool {
					serialReadHandler(args, seq, buf, len, err);
					return true;
				};
				if ( ! args.framing->read(recvBuffer[i], readHandlerMapping) ) {
					args.callback->onVkvmBrokenFrame();
				}
				if (args.serial == NULL) {
					return; /* read handler terminated the connection */
				}
			}
			/* ping device if no request was sent for timeout milliseconds */
			const unsigned long now = millis();
			if (size_t(now - args.lastSent) >= args.timeout) {
				serialQueueCommand<void>(args, RequestType::GET_ALIVE, NULL);
			}
			/* send outstanding request first before checking timeouts */
			if ( ! args.reqPending ) continue;
			/* check timeouts of pending requests */
			args.queueMutex.lock();
			if (size_t(now - args.lastSent) >= args.timeout) {
				args.queueMutex.unlock();
				vkvmTrace(3, "timeout2\t%lu\n", args.lastSent);
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_TIMEOUT);
				return;
			}
			args.queueMutex.unlock();
		}
		if ( args.terminate ) {
			serialDisconnect(args, VkvmCallback::DisconnectReason::D_USER);
		}
	} catch (...) {}
}


/**
 * Background thread which writes outstanding requests to the serial connected
 * VKVM periphery. This performs one request at a time. The actual serialization
 * of the queued requests is done in `RequestQueueItemT::send()`.
 *
 * @param[in] args - object with the shared `VkvmDevice` arguments
 */
static void serialWriteThread(SerialCommon & args) {
	try {
		std::lock_guard<std::mutex> writeGuard(args.writeMutex);
		if (args.device == NULL || args.framing == NULL || args.callback == NULL || args.serial == NULL || args.terminate) {
			if ( args.terminate ) {
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_USER);
			} else {
				serialDisconnect(args, VkvmCallback::DisconnectReason::D_SEND_ERROR);
			}
			return;
		}
		while ( ! args.terminate ) {
			/* send next request if none is pending */
			std::unique_lock<std::mutex> guard(args.queueMutex);
			args.writable.wait(guard, [&args] {
				return args.terminate || args.serial == NULL || (args.reqPending == false && args.reqFifoFirst != NULL);
			});
			if (args.terminate || args.serial == NULL) return;
			if (args.reqPending == false && args.reqFifoFirst != NULL) {
				if ( ! args.reqFifoFirst->send(args) ) {
					args.queueMutex.unlock();
					serialDisconnect(args, VkvmCallback::DisconnectReason::D_SEND_ERROR);
					return;
				}
			}
		}
	} catch (...) {}
}


#ifdef PCF_IS_LINUX
/** Holds a single input device handle. */
struct InputDevice {
	typedef long ValueType;
	char * path; /**< Associated device path for error output. */
	struct libinput_device * liDev; /**< Associated libinput device handle. */
	int fd; /**< File descriptor for the input device. */
	bool failed; /**< Failed to capture/grab input? */
	bool grabbed; /**< Is the input captured/grabbed? */
	/** Default constructor. */
	explicit inline InputDevice():
		path(NULL),
		liDev(NULL),
		fd(-1),
		failed(false),
		grabbed(false)
	{}
	/** Destructor. */
	inline ~InputDevice() {
		if (fd >= 0 && grabbed) {
			errno = 0;
			if (xEINTR(ioctl, fd, EVIOCGRAB, GRAB_OFF) >= 0) { /* ungrab */
				vkvmTrace(3, "ungrab\t%s\n", path);
			} else {
				vkvmTrace(3, "error\tungrab\t%s\t%s\n", path, strerror(errno));
			}
		}
		if (liDev != NULL) libinput_device_unref(liDev);
		if (path != NULL) free(path);
	}
	/**
	 * Round the given double to `ValueType`.
	 *
	 * @param[in] val - value to round
	 * @return rounded value
	 */
	static inline ValueType round(const double val) {
		return ValueType((val < 0.0) ? val - 0.5 : val + 0.5);
	}
	/**
	 * Round the given double to absolute pointer coordinates.
	 *
	 * @param[in] val - value to round
	 * @return rounded value
	 */
	static inline int16_t roundAbs(const double val) {
		return int16_t((val < 0.0) ? 0.0 : (val > 32766.5) ? 32767.0 : val + 0.5f);
	}
};
#endif /* PCF_IS_LINUX */


} /* anonymous namespace */


struct VkvmDevice::Pimple {
	SerialCommon common; /**< object with the shared `VkvmDevice` arguments */
	std::thread readThread; /**< background serial read thread handle */
	std::thread writeThread; /**< background serial write thread handle */
	bool grabbingInput; /** true if the keyboard/mouse events are being globally captured, else false */
};


/** Constructor. */
VkvmDevice::VkvmDevice():
	self(new VkvmDevice::Pimple)
{
	self->common.device = this;
	self->common.framing = new Framing<VKVM_MAX_FRAME_SIZE>(serialWrite, &(self->common));
	self->common.bufferSize = 0;
	self->common.reqFifoFirst = NULL;
	self->common.reqFifoLast = NULL;
	self->common.reqFifoSize = 0;
	self->common.reqNumber = 0;
	self->common.reqPending = false;
	self->common.tickDuration = 100;
	self->common.timeout = 1000;
	self->common.callback = NULL;
	self->common.serial = NULL;
	self->common.terminate = false;
	self->common.lastUsbState = USBSTATE_OFF;
	self->common.lastLEDs = 0;
#if defined(PCF_IS_WIN)
	self->common.keyboardHook = NULL;
	self->common.mouseHook = NULL;
#elif defined(PCF_IS_LINUX)
	self->common.hookTermFd = -1;
#endif /* PCF_IS_LINUX */
	self->grabbingInput = false;
}


/** Destructor. */
VkvmDevice::~VkvmDevice() {
	try {
		this->close();
		if (self == NULL) return;
		if (self->common.framing != NULL) delete self->common.framing;
		if (self->common.serial != NULL) ser_delete(self->common.serial);
		delete this->self;
	} catch (...) {}
}


/**
 * Opens the given VKVM periphery device from the passed serial device path.
 *
 * @param[in,out] cb - reference to a callback handler used for device changes
 *  notifications (needs to be valid over the life-time of this object)
 * @param[in] path - serial device path of the VKVM periphery device
 * @param[in] timeout - timeout in milliseconds
 * @param[in] tickDuration - internal serial read timeout in milliseconds used
 *  to check the VKVM periphery device via keep-alive
 * @return true on success, else false
 */
bool VkvmDevice::open(VkvmCallback & cb, const char * path, const size_t timeout, const size_t tickDuration) {
	vkvmTrace(3, "open\t%s\n", path);
	std::lock_guard<std::mutex> guard(self->common.openCloseMutex);
	if (self->common.serial != NULL || self->common.terminate) return false;
	if ( self->common.disconnectThread.joinable() ) self->common.disconnectThread.join();
	if ( self->readThread.joinable() ) self->readThread.join();
	if ( self->writeThread.joinable() ) self->writeThread.join();
	unsigned long startTime = millis();
	size_t elapsed;
	tSerError serRes = SE_SUCCESS;
	do {
		self->common.serial = ser_create(path, VKVM_PROT_SPEED, SFR_8N1, SFC_NONE);
		serRes = ser_lastError();
		elapsed = size_t(millis() - startTime);
	} while (self->common.serial == NULL && (serRes == SE_TIMEOUT || serRes == SE_BUSY) && elapsed < timeout);
	if (self->common.serial == NULL) {
		VkvmCallback::DisconnectReason reason = VkvmCallback::DisconnectReason::D_SEND_ERROR;
		if (elapsed >= timeout) {
			reason = VkvmCallback::DisconnectReason::D_TIMEOUT;
		}
		vkvmTrace(3, "disconnect\t%i\n", int(reason));
		vkvmTrace(3, "disconnect forwarded\t%i\n", int(reason));
		cb.onVkvmDisconnected(reason);
		return false;
	}
	ser_clear(self->common.serial);
	self->common.framing->setFirstOut();
	self->common.bufferSize = 0;
	while (self->common.reqFifoFirst != NULL) {
		RequestQueueItem * next = self->common.reqFifoFirst->next;
		delete self->common.reqFifoFirst;
		self->common.reqFifoFirst = next;
	}
	self->common.reqFifoFirst = NULL;
	self->common.reqFifoLast = NULL;
	self->common.reqFifoSize = 0;
	self->common.reqNumber = 0;
	self->common.reqPending = false;
	self->common.lastUsbState = USBSTATE_OFF;
	self->common.lastLEDs = 0;
	self->common.tickDuration = tickDuration;
	self->common.timeout = timeout;
	self->common.callback = &cb;
	self->common.connected = false;
	self->readThread = std::thread(serialReadThread, std::ref(self->common));
	self->writeThread = std::thread(serialWriteThread, std::ref(self->common));
	vkvmTrace(3, "opened\t0x%p\t%s\n", static_cast<const void *>(self->common.serial), path);
	return true;
}


/**
 * Checks whether the serial connection has been established.
 *
 * @return true if serial device has been opened, else false
 */
bool VkvmDevice::isOpen() const {
	return (self->common.serial != NULL);
}


/**
 * Checks whether the serial device has been opened and the VKVM periphery
 * has been connected. Both ends need to be properly connected to return
 * true here.
 *
 * @return true for a fully established link to the VKVM periphery, else false
 */
bool VkvmDevice::isConnected() const {
	return (self->common.serial != NULL && self->common.connected);
}


/**
 * Checks whether the serial device has been opened and the VKVM periphery
 * has been connected. Also tests whether the periphery USB connection is
 * up and running. Both ends and the remote USB interface need to be properly
 * connected to return true here.
 *
 * @return true for a fully established link to the remotely connected device, else false
 */
bool VkvmDevice::isFullyConnected() const {
	return (self->common.serial != NULL && self->common.connected && self->common.lastUsbState == USBSTATE_ON_CONFIGURED);
}


/**
 * Closed the serial connection to the VKVM periphery device.
 *
 * @return true on success, else false
 */
bool VkvmDevice::close() {
	vkvmTrace(3, "close1\t0x%p\n", static_cast<const void *>(self->common.serial));
	std::lock_guard<std::mutex> guard(self->common.openCloseMutex);
	if (self->common.serial == NULL || self->common.terminate) {
		if ( self->common.disconnectThread.joinable() ) self->common.disconnectThread.join();
		if ( self->readThread.joinable() ) self->readThread.join();
		if ( self->writeThread.joinable() ) self->writeThread.join();
		return false;
	}
	self->common.terminate = true;
	self->common.writable.notify_one();
	if ( self->grabbingInput ) this->grabGlobalInput(false);
	if ( self->common.disconnectThread.joinable() ) self->common.disconnectThread.join();
	if ( self->readThread.joinable() ) self->readThread.join();
	if ( self->writeThread.joinable() ) self->writeThread.join();
	self->common.callback = NULL;
	const void * oldHandle = self->common.serial;
	PCF_UNUSED(oldHandle);
	ser_delete(self->common.serial);
	self->common.serial = NULL;
	self->common.terminate = false;
	vkvmTrace(3, "closed1\t0x%p\n", oldHandle);
	return true;
}


/**
 * Returns the most recent USB periphery state field from
 * the connected remote device.
 *
 * @return USB periphry state field
 * @see vkm-periphery/UsbKeys.hpp (e.g. `USBSTATE_OFF`)
 */
uint8_t VkvmDevice::usbState() const {
	return self->common.lastUsbState;
}


/**
 * Returns the most recent keyboard LED bit field from
 * the connected remote device.
 *
 * @return LED bit field
 * @see vkm-periphery/UsbKeys.hpp (e.g. `USBLED_NUM_LOCK`)
 */
uint8_t VkvmDevice::keyboardLeds() const {
	return self->common.lastLEDs;
}


/**
 * Sends a keyboard key down event to the connected remote
 * device.
 *
 * @param[in] key - key to press down (e.g. `USBKEY_LEFT_CONTROL`)
 * @param[in] osKey - optional OS specific key (passed to `onVkvmRemapKey()`)
 * @return true on success, else false
 */
bool VkvmDevice::keyboardDown(const uint8_t key, const int osKey) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newKey = self->common.callback->onVkvmRemapKey(key, osKey, VkvmCallback::RemapFor::RF_DOWN);
	if (newKey == USBKEY_NO_EVENT) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_KEYBOARD_DOWN, &VkvmCallback::onVkvmKeyboardDown, newKey);
}


/**
 * Sends a keyboard key release event to the connected remote
 * device.
 *
 * @param[in] key - key to release (e.g. `USBKEY_LEFT_CONTROL`)
 * @param[in] osKey - optional OS specific key (passed to `onVkvmRemapKey()`)
 * @return true on success, else false
 */
bool VkvmDevice::keyboardUp(const uint8_t key, const int osKey) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newKey = self->common.callback->onVkvmRemapKey(key, osKey, VkvmCallback::RemapFor::RF_UP);
	if (newKey == USBKEY_NO_EVENT) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_KEYBOARD_UP, &VkvmCallback::onVkvmKeyboardUp, newKey);
}


/**
 * Sends a keyboard key release event to the connected remote
 * device for all keys pressed down.
 *
 * @return true on success, else false
 */
bool VkvmDevice::keyboardAllUp() {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_KEYBOARD_ALL_UP, &VkvmCallback::onVkvmKeyboardAllUp);
}


/**
 * Sends a keyboard key push event to the connected remote
 * device. This consists of a key down followed by a key
 * release event.
 *
 * @param[in] key - key to push (e.g. `USBKEY_LEFT_CONTROL`)
 * @param[in] osKey - optional OS specific key (passed to `onVkvmRemapKey()`)
 * @return true on success, else false
 */
bool VkvmDevice::keyboardPush(const uint8_t key, const int osKey) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newKey = self->common.callback->onVkvmRemapKey(key, osKey, VkvmCallback::RemapFor::RF_PUSH);
	if (newKey == USBKEY_NO_EVENT) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_KEYBOARD_PUSH, &VkvmCallback::onVkvmKeyboardPush, newKey);
}


/**
 * Sends multiple keyboard key push events to the connected
 * remote device. Each consists of a key down followed by a
 * key release event. Note that key modifiers within the key
 * array are applied instead of being sent as separate key
 * events.
 *
 * @param[in] mod - initial key modifier (e.g. `USBWRITE_LEFT_CONTROL`)
 * @param[in] keys - array of key values (single key example value is `USBKEY_A`)
 * @param[in] len - number of keys in the array
 * @return true on success, else false
 */
bool VkvmDevice::keyboardWrite(const uint8_t mod, const uint8_t * keys, const uint8_t len) {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_KEYBOARD_WRITE, &VkvmCallback::onVkvmKeyboardWrite, mod, ByteBuffer(keys, len));
}


/**
 * Sends a mouse button down event to the connected remote
 * device.
 *
 * @param[in] button - mouse button to press down (e.g. `USBBUTTON_LEFT`)
 * @return true on success, else false
 */
bool VkvmDevice::mouseButtonDown(const uint8_t button) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newButton = self->common.callback->onVkvmRemapButton(button, VkvmCallback::RemapFor::RF_DOWN);
	if (newButton == 0) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_BUTTON_DOWN, &VkvmCallback::onVkvmMouseButtonDown, newButton);
}


/**
 * Sends a mouse button release event to the connected remote
 * device.
 *
 * @param[in] button - mouse button to release (e.g. `USBBUTTON_LEFT`)
 * @return true on success, else false
 */
bool VkvmDevice::mouseButtonUp(const uint8_t button) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newButton = self->common.callback->onVkvmRemapButton(button, VkvmCallback::RemapFor::RF_UP);
	if (newButton == 0) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_BUTTON_UP, &VkvmCallback::onVkvmMouseButtonUp, newButton);
}


/**
 * Sends a mouse button release event to the connected remote
 * device for all mouse buttons pressed down.
 *
 * @return true on success, else false
 */
bool VkvmDevice::mouseButtonAllUp() {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_BUTTON_ALL_UP, &VkvmCallback::onVkvmMouseButtonAllUp);
}


/**
 * Sends a mouse button push event to the connected remote
 * device. This consists of a mouse button down followed by
 * a mouse button release event.
 *
 * @param[in] button - mouse button to push (e.g. `USBBUTTON_LEFT`)
 * @return true on success, else false
 */
bool VkvmDevice::mouseButtonPush(const uint8_t button) {
	if ( ! this->isOpen() ) return false;
	const uint8_t newButton = self->common.callback->onVkvmRemapButton(button, VkvmCallback::RemapFor::RF_PUSH);
	if (newButton == 0) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_BUTTON_PUSH, &VkvmCallback::onVkvmMouseButtonPush, newButton);
}


/**
 * Sends the new absolute mouse pointer coordinates to the
 * connected remote device. Note that this value is based on
 * the current screen resolution and measures as a fraction
 * of it. The left bottom corner is located at `0, 0` and the
 * right upper corner at `32767, 32767`.
 *
 * @param[in] x - on-screen x coordinate
 * @param[in] y - on-screen y coordinate
 * @return true on success, else false
 */
bool VkvmDevice::mouseMoveAbs(const int16_t x, const int16_t y) {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_MOVE_ABS, &VkvmCallback::onVkvmMouseMoveAbs, x, y);
}


/**
 * Sends a mouse pointer coordinate delta event to the
 * connected remote device.
 *
 * @param[in] x - delta on the x axis in pixels +/- 127
 * @param[in] y - delta on the y axis in pixels +/- 127
 * @return true on success, else false
 */
bool VkvmDevice::mouseMoveRel(const int8_t x, const int8_t y) {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_MOVE_REL, &VkvmCallback::onVkvmMouseMoveRel, x, y);
}


/**
 * Sends a mouse wheel delta event to the connected
 * remote device.
 *
 * @param[in] wheel - mouse wheel delta +/- 127
 * @return true on success, else false
 */
bool VkvmDevice::mouseScroll(const int8_t wheel) {
	if ( ! this->isOpen() ) return false;
	return serialQueueCommand<void>(self->common, RequestType::SET_MOUSE_SCROLL, &VkvmCallback::onVkvmMouseScroll, wheel);
}


#if defined(PCF_IS_WIN)
/* There can be only one global grab active at a time. */
static SerialCommon * vkvmHookCtx = NULL;


/**
 * Maps a Windows scan code (index) to a USB key code.
 *
 * @see http://www.quadibloc.com/comp/scan.htm escape is encoded as (SC OR 0x80)
 * @see https://sourceforge.net/p/vice-emu/feature-requests/_discuss/thread/56d8c4b0/dd88/attachment/scancodes%20Set%201.pdf
 * @see https://web.archive.org/web/20190301075756/https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/scancode.doc
 */
static const uint8_t scMap[222] = {
	USBKEY_NO_EVENT, /* 0x00 */
	USBKEY_ESCAPE, /* 0x01 */
	USBKEY_1, /* 0x02 */
	USBKEY_2, /* 0x03 */
	USBKEY_3, /* 0x04 */
	USBKEY_4, /* 0x05 */
	USBKEY_5, /* 0x06 */
	USBKEY_6, /* 0x07 */
	USBKEY_7, /* 0x08 */
	USBKEY_8, /* 0x09 */
	USBKEY_9, /* 0x0A */
	USBKEY_0, /* 0x0B */
	USBKEY_MINUS, /* 0x0C */
	USBKEY_EQUAL, /* 0x0D */
	USBKEY_BACKSPACE, /* 0x0E */
	USBKEY_TAB, /* 0x0F */
	USBKEY_Q, /* 0x10 */
	USBKEY_W, /* 0x11 */
	USBKEY_E, /* 0x12 */
	USBKEY_R, /* 0x13 */
	USBKEY_T, /* 0x14 */
	USBKEY_Y, /* 0x15 */
	USBKEY_U, /* 0x16 */
	USBKEY_I, /* 0x17 */
	USBKEY_O, /* 0x18 */
	USBKEY_P, /* 0x19 */
	USBKEY_OPEN_BRACKET, /* 0x1A */
	USBKEY_CLOSE_BRACKET, /* 0x1B */
	USBKEY_ENTER, /* 0x1C */
	USBKEY_LEFT_CONTROL, /* 0x1D */
	USBKEY_A, /* 0x1E */
	USBKEY_S, /* 0x1F */
	USBKEY_D, /* 0x20 */
	USBKEY_F, /* 0x21 */
	USBKEY_G, /* 0x22 */
	USBKEY_H, /* 0x23 */
	USBKEY_J, /* 0x24 */
	USBKEY_K, /* 0x25 */
	USBKEY_L, /* 0x26 */
	USBKEY_SEMICOLON, /* 0x27 */
	USBKEY_APOSTROPHE, /* 0x28 */
	USBKEY_ACCENT, /* 0x29 */
	USBKEY_LEFT_SHIFT, /* 0x2A */
	USBKEY_BACKSLASH, /* 0x2B, also USBKEY_NON_US_HASH */
	USBKEY_Z, /* 0x2C */
	USBKEY_X, /* 0x2D */
	USBKEY_C, /* 0x2E */
	USBKEY_V, /* 0x2F */
	USBKEY_B, /* 0x30 */
	USBKEY_N, /* 0x31 */
	USBKEY_M, /* 0x32 */
	USBKEY_COMMA, /* 0x33 */
	USBKEY_PERIOD, /* 0x34 */
	USBKEY_SLASH, /* 0x35 */
	USBKEY_RIGHT_SHIFT, /* 0x36 */
	USBKEY_KP_MULTIPLY, /* 0x37 */
	USBKEY_LEFT_ALT, /* 0x38 */
	USBKEY_SPACE, /* 0x39 */
	USBKEY_CAPS_LOCK, /* 0x3A */
	USBKEY_F1, /* 0x3B */
	USBKEY_F2, /* 0x3C */
	USBKEY_F3, /* 0x3D */
	USBKEY_F4, /* 0x3E */
	USBKEY_F5, /* 0x3F */
	USBKEY_F6, /* 0x40 */
	USBKEY_F7, /* 0x41 */
	USBKEY_F8, /* 0x42 */
	USBKEY_F9, /* 0x43 */
	USBKEY_F10, /* 0x44 */
	USBKEY_PAUSE, /* 0x45 */
	USBKEY_SCROLL_LOCK, /* 0x46 */
	USBKEY_KP_7, /* 0x47 */
	USBKEY_KP_8, /* 0x48 */
	USBKEY_KP_9, /* 0x49 */
	USBKEY_KP_SUBTRACT, /* 0x4A */
	USBKEY_KP_4, /* 0x4B */
	USBKEY_KP_5, /* 0x4C */
	USBKEY_KP_6, /* 0x4D */
	USBKEY_KP_ADD, /* 0x4E */
	USBKEY_KP_1, /* 0x4F */
	USBKEY_KP_2, /* 0x50 */
	USBKEY_KP_3, /* 0x51 */
	USBKEY_KP_0, /* 0x52 */
	USBKEY_KP_DECIMAL, /* 0x53 */
	USBKEY_ATTN, /* 0x54 */
	USBKEY_NO_EVENT, /* 0x55 */
	USBKEY_NON_US_BACKSLASH, /* 0x56 */
	USBKEY_F11, /* 0x57 */
	USBKEY_F12, /* 0x58 */
	USBKEY_KP_EQUAL, /* 0x59 */
	USBKEY_NO_EVENT, /* 0x5A */
	USBKEY_NO_EVENT, /* 0x5B */
	USBKEY_INT_6, /* 0x5C */
	USBKEY_NO_EVENT, /* 0x5D */
	USBKEY_NO_EVENT, /* 0x5E */
	USBKEY_NO_EVENT, /* 0x5F */
	USBKEY_NO_EVENT, /* 0x60 */
	USBKEY_NO_EVENT, /* 0x61 */
	USBKEY_NO_EVENT, /* 0x62 */
	USBKEY_NO_EVENT, /* 0x63 */
	USBKEY_F13, /* 0x64 */
	USBKEY_F14, /* 0x65 */
	USBKEY_F15, /* 0x66 */
	USBKEY_F16, /* 0x67 */
	USBKEY_F17, /* 0x68 */
	USBKEY_F18, /* 0x69 */
	USBKEY_F19, /* 0x6A */
	USBKEY_F20, /* 0x6B */
	USBKEY_F21, /* 0x6C */
	USBKEY_F22, /* 0x6D */
	USBKEY_F23, /* 0x6E */
	USBKEY_NO_EVENT, /* 0x6F */
	USBKEY_INT_2, /* 0x70 */
	USBKEY_NO_EVENT, /* 0x71 */
	USBKEY_NO_EVENT, /* 0x72 */
	USBKEY_INT_1, /* 0x73 */
	USBKEY_NO_EVENT, /* 0x74 */
	USBKEY_NO_EVENT, /* 0x75 */
	USBKEY_LANG_5, /* 0x76, also USBKEY_F24 */
	USBKEY_LANG_4, /* 0x77 */
	USBKEY_LANG_3, /* 0x78 */
	USBKEY_INT_4, /* 0x79 */
	USBKEY_NO_EVENT, /* 0x7A */
	USBKEY_INT_5, /* 0x7B */
	USBKEY_NO_EVENT, /* 0x7C */
	USBKEY_INT_3, /* 0x7D */
	USBKEY_KP_COMMA, /* 0x7E */
	USBKEY_NO_EVENT, /* 0x7F */
	USBKEY_NO_EVENT, /* 0x80 */
	USBKEY_NO_EVENT, /* 0x81 */
	USBKEY_NO_EVENT, /* 0x82 */
	USBKEY_NO_EVENT, /* 0x83 */
	USBKEY_NO_EVENT, /* 0x84 */
	USBKEY_NO_EVENT, /* 0x85 */
	USBKEY_NO_EVENT, /* 0x86 */
	USBKEY_NO_EVENT, /* 0x87 */
	USBKEY_NO_EVENT, /* 0x88 */
	USBKEY_NO_EVENT, /* 0x89 */
	USBKEY_NO_EVENT, /* 0x8A */
	USBKEY_NO_EVENT, /* 0x8B */
	USBKEY_NO_EVENT, /* 0x8C */
	USBKEY_NO_EVENT, /* 0x8D */
	USBKEY_NO_EVENT, /* 0x8E */
	USBKEY_NO_EVENT, /* 0x8F */
	USBKEY_NO_EVENT, /* 0x90 */
	USBKEY_NO_EVENT, /* 0x91 */
	USBKEY_NO_EVENT, /* 0x92 */
	USBKEY_NO_EVENT, /* 0x93 */
	USBKEY_NO_EVENT, /* 0x94 */
	USBKEY_NO_EVENT, /* 0x95 */
	USBKEY_NO_EVENT, /* 0x96 */
	USBKEY_NO_EVENT, /* 0x97 */
	USBKEY_NO_EVENT, /* 0x98 */
	USBKEY_NO_EVENT, /* 0x99 */
	USBKEY_NO_EVENT, /* 0x9A */
	USBKEY_NO_EVENT, /* 0x9B */
	USBKEY_KP_ENTER, /* 0x9C */
	USBKEY_RIGHT_CONTROL, /* 0x9D */
	USBKEY_NO_EVENT, /* 0x9E */
	USBKEY_NO_EVENT, /* 0x9F */
	USBKEY_NO_EVENT, /* 0xA0 */
	USBKEY_NO_EVENT, /* 0xA1 */
	USBKEY_NO_EVENT, /* 0xA2 */
	USBKEY_NO_EVENT, /* 0xA3 */
	USBKEY_NO_EVENT, /* 0xA4 */
	USBKEY_NO_EVENT, /* 0xA5 */
	USBKEY_NO_EVENT, /* 0xA6 */
	USBKEY_NO_EVENT, /* 0xA7 */
	USBKEY_NO_EVENT, /* 0xA8 */
	USBKEY_NO_EVENT, /* 0xA9 */
	USBKEY_NO_EVENT, /* 0xAA */
	USBKEY_NO_EVENT, /* 0xAB */
	USBKEY_NO_EVENT, /* 0xAC */
	USBKEY_NO_EVENT, /* 0xAD */
	USBKEY_NO_EVENT, /* 0xAE */
	USBKEY_NO_EVENT, /* 0xAF */
	USBKEY_NO_EVENT, /* 0xB0 */
	USBKEY_NO_EVENT, /* 0xB1 */
	USBKEY_NO_EVENT, /* 0xB2 */
	USBKEY_NO_EVENT, /* 0xB3 */
	USBKEY_NO_EVENT, /* 0xB4 */
	USBKEY_KP_DIVIDE, /* 0xB5 */
	USBKEY_RIGHT_SHIFT, /* 0xB6 */
	USBKEY_PRINT_SCREEN, /* 0xB7 */
	USBKEY_RIGHT_ALT, /* 0xB8 */
	USBKEY_NO_EVENT, /* 0xB9 */
	USBKEY_NO_EVENT, /* 0xBA */
	USBKEY_NO_EVENT, /* 0xBB */
	USBKEY_NO_EVENT, /* 0xBC */
	USBKEY_NO_EVENT, /* 0xBD */
	USBKEY_NO_EVENT, /* 0xBE */
	USBKEY_NO_EVENT, /* 0xBF */
	USBKEY_NO_EVENT, /* 0xC0 */
	USBKEY_NO_EVENT, /* 0xC1 */
	USBKEY_NO_EVENT, /* 0xC2 */
	USBKEY_NO_EVENT, /* 0xC3 */
	USBKEY_NO_EVENT, /* 0xC4 */
	USBKEY_NUM_LOCK, /* 0xC5 */
	USBKEY_NO_EVENT, /* 0xC6 */
	USBKEY_HOME, /* 0xC7 */
	USBKEY_UP_ARROW, /* 0xC8 */
	USBKEY_PAGE_UP, /* 0xC9 */
	USBKEY_NO_EVENT, /* 0xCA */
	USBKEY_LEFT_ARROW, /* 0xCB */
	USBKEY_NO_EVENT, /* 0xCC */
	USBKEY_RIGHT_ARROW, /* 0xCD */
	USBKEY_NO_EVENT, /* 0xCE */
	USBKEY_END, /* 0xCF */
	USBKEY_DOWN_ARROW, /* 0xD0 */
	USBKEY_PAGE_DOWN, /* 0xD1 */
	USBKEY_INSERT, /* 0xD2 */
	USBKEY_DELETE, /* 0xD3 */
	USBKEY_NO_EVENT, /* 0xD4 */
	USBKEY_NO_EVENT, /* 0xD5 */
	USBKEY_NO_EVENT, /* 0xD6 */
	USBKEY_NO_EVENT, /* 0xD7 */
	USBKEY_NO_EVENT, /* 0xD8 */
	USBKEY_NO_EVENT, /* 0xD9 */
	USBKEY_NO_EVENT, /* 0xDA */
	USBKEY_LEFT_GUI, /* 0xDB */
	USBKEY_RIGHT_GUI, /* 0xDC */
	USBKEY_APPLICATION /* 0xDD */
};


/**
 * Low level keyboard hook callback.
 *
 * @param[in] nCode - event processing type
 * @param[in] wParam - identifier of the keyboard message
 * @param[in] lParam - pointer to `KBDLLHOOKSTRUCT`
 * @return `CallNextHookEx()` result if `nCode < 0` or unprocessed, else non-zero if processed
 * @see https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644985(v=vs.85)
 */
LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || vkvmHookCtx == NULL) return CallNextHookEx(NULL, nCode, wParam, lParam);
	PKBDLLHOOKSTRUCT p = reinterpret_cast<PKBDLLHOOKSTRUCT>(lParam);
	switch (wParam) {
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
		const int scanCode = int(p->scanCode | ((p->flags << 7) & 0x80));
		if (size_t(scanCode) < sizeof(scMap)) {
			const uint8_t key = scMap[size_t(scanCode)];
			if ((p->flags & 0x80) == 0) { /* key was pressed */
				if ( ! vkvmHookCtx->device->keyboardDown(key, scanCode) ) return 1;
			} else {
				if ( ! vkvmHookCtx->device->keyboardUp(key, scanCode) ) return 1;
			}
		}
		} break;
	default: break;
	}
	return 1; /* remove the message from the message queue */
}


/**
 * Low level mouse hook callback.
 *
 * @param[in] nCode - event processing type
 * @param[in] wParam - identifier of the mouse message
 * @param[in] lParam - pointer to `MSLLHOOKSTRUCT`
 * @return `CallNextHookEx()` result if `nCode < 0` or unprocessed, else non-zero if processed
 * @see https://learn.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms644986(v=vs.85)
 */
LRESULT CALLBACK lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode != HC_ACTION || vkvmHookCtx == NULL) return CallNextHookEx(NULL, nCode, wParam, lParam);
	PMSLLHOOKSTRUCT p = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);
	switch (wParam) {
	case WM_LBUTTONDOWN: vkvmHookCtx->device->mouseButtonDown(USBBUTTON_LEFT);   break;
	case WM_LBUTTONUP:   vkvmHookCtx->device->mouseButtonUp(USBBUTTON_LEFT);     break;
	case WM_RBUTTONDOWN: vkvmHookCtx->device->mouseButtonDown(USBBUTTON_RIGHT);  break;
	case WM_RBUTTONUP:   vkvmHookCtx->device->mouseButtonUp(USBBUTTON_RIGHT);    break;
	case WM_MBUTTONDOWN: vkvmHookCtx->device->mouseButtonDown(USBBUTTON_MIDDLE); break;
	case WM_MBUTTONUP:   vkvmHookCtx->device->mouseButtonUp(USBBUTTON_MIDDLE);   break;
	case WM_MOUSEMOVE:
		if ( vkvmHookCtx->hasLastMouse ) {
			long deltaMouseX = p->pt.x - vkvmHookCtx->lastMouseX;
			long deltaMouseY = p->pt.y - vkvmHookCtx->lastMouseY;
			while (deltaMouseX != 0 || deltaMouseY != 0) {
				const long moveX = PCF_MIN(PCF_MAX(deltaMouseX, -127), 127);
				const long moveY = PCF_MIN(PCF_MAX(deltaMouseY, -127), 127);
				if ( ! vkvmHookCtx->device->mouseMoveRel(int8_t(moveX), int8_t(moveY)) ) return 1;
				deltaMouseX -= moveX;
				deltaMouseY -= moveY;
			}
		} else {
			POINT lpPoint;
			GetCursorPos(&lpPoint);
			vkvmHookCtx->lastMouseX = long(lpPoint.x);
			vkvmHookCtx->lastMouseY = long(lpPoint.y);
			vkvmHookCtx->hasLastMouse = true;
		}
		break;
	case WM_MOUSEWHEEL:
		{
			long deltaMouseWheel = GET_WHEEL_DELTA_WPARAM(p->mouseData) / WHEEL_DELTA;
			while (deltaMouseWheel != 0) {
				const long moveWheel = PCF_MIN(PCF_MAX(deltaMouseWheel, -127), 127);
				if ( ! vkvmHookCtx->device->mouseScroll(int8_t(moveWheel)) ) return 1;
				deltaMouseWheel -= moveWheel;
			}
		}
		break;
	default: break;
	}
	return 1; /* remove the message from the message queue */
}
#elif defined(PCF_IS_LINUX)
/**
 * Maps the given Linux key code value to a USB key value.
 *
 * @param[in] osKey - Linux key code
 * @return USB key code
 * @see https://docs.kernel.org/input/event-codes.html
 * @see https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
 */
static uint8_t mapKeyCode(const int osKey) {
	uint8_t res;
	switch (osKey) {
	case KEY_RESERVED:         res = USBKEY_NO_EVENT;          break;
	case KEY_ESC:              res = USBKEY_ESCAPE;            break;
	case KEY_1:                res = USBKEY_1;                 break;
	case KEY_2:                res = USBKEY_2;                 break;
	case KEY_3:                res = USBKEY_3;                 break;
	case KEY_4:                res = USBKEY_4;                 break;
	case KEY_5:                res = USBKEY_5;                 break;
	case KEY_6:                res = USBKEY_6;                 break;
	case KEY_7:                res = USBKEY_7;                 break;
	case KEY_8:                res = USBKEY_8;                 break;
	case KEY_9:                res = USBKEY_9;                 break;
	case KEY_0:                res = USBKEY_0;                 break;
	case KEY_MINUS:            res = USBKEY_MINUS;             break;
	case KEY_EQUAL:            res = USBKEY_EQUAL;             break;
	case KEY_BACKSPACE:        res = USBKEY_BACKSPACE;         break;
	case KEY_TAB:              res = USBKEY_TAB;               break;
	case KEY_Q:                res = USBKEY_Q;                 break;
	case KEY_W:                res = USBKEY_W;                 break;
	case KEY_E:                res = USBKEY_E;                 break;
	case KEY_R:                res = USBKEY_R;                 break;
	case KEY_T:                res = USBKEY_T;                 break;
	case KEY_Y:                res = USBKEY_Y;                 break;
	case KEY_U:                res = USBKEY_U;                 break;
	case KEY_I:                res = USBKEY_I;                 break;
	case KEY_O:                res = USBKEY_O;                 break;
	case KEY_P:                res = USBKEY_P;                 break;
	case KEY_LEFTBRACE:        res = USBKEY_OPEN_BRACKET;      break;
	case KEY_RIGHTBRACE:       res = USBKEY_CLOSE_BRACKET;     break;
	case KEY_ENTER:            res = USBKEY_ENTER;             break;
	case KEY_LEFTCTRL:         res = USBKEY_LEFT_CONTROL;      break;
	case KEY_A:                res = USBKEY_A;                 break;
	case KEY_S:                res = USBKEY_S;                 break;
	case KEY_D:                res = USBKEY_D;                 break;
	case KEY_F:                res = USBKEY_F;                 break;
	case KEY_G:                res = USBKEY_G;                 break;
	case KEY_H:                res = USBKEY_H;                 break;
	case KEY_J:                res = USBKEY_J;                 break;
	case KEY_K:                res = USBKEY_K;                 break;
	case KEY_L:                res = USBKEY_L;                 break;
	case KEY_SEMICOLON:        res = USBKEY_SEMICOLON;         break;
	case KEY_APOSTROPHE:       res = USBKEY_APOSTROPHE;        break;
	case KEY_GRAVE:            res = USBKEY_ACCENT;            break;
	case KEY_LEFTSHIFT:        res = USBKEY_LEFT_SHIFT;        break;
	case KEY_BACKSLASH:        res = USBKEY_BACKSLASH;         break;
	case KEY_Z:                res = USBKEY_Z;                 break;
	case KEY_X:                res = USBKEY_X;                 break;
	case KEY_C:                res = USBKEY_C;                 break;
	case KEY_V:                res = USBKEY_V;                 break;
	case KEY_B:                res = USBKEY_B;                 break;
	case KEY_N:                res = USBKEY_N;                 break;
	case KEY_M:                res = USBKEY_M;                 break;
	case KEY_COMMA:            res = USBKEY_COMMA;             break;
	case KEY_DOT:              res = USBKEY_PERIOD;            break;
	case KEY_SLASH:            res = USBKEY_SLASH;             break;
	case KEY_RIGHTSHIFT:       res = USBKEY_RIGHT_SHIFT;       break;
	case KEY_KPASTERISK:       res = USBKEY_KP_MULTIPLY;       break;
	case KEY_LEFTALT:          res = USBKEY_LEFT_ALT;          break;
	case KEY_SPACE:            res = USBKEY_SPACE;             break;
	case KEY_CAPSLOCK:         res = USBKEY_CAPS_LOCK;         break;
	case KEY_F1:               res = USBKEY_F1;                break;
	case KEY_F2:               res = USBKEY_F2;                break;
	case KEY_F3:               res = USBKEY_F3;                break;
	case KEY_F4:               res = USBKEY_F4;                break;
	case KEY_F5:               res = USBKEY_F5;                break;
	case KEY_F6:               res = USBKEY_F6;                break;
	case KEY_F7:               res = USBKEY_F7;                break;
	case KEY_F8:               res = USBKEY_F8;                break;
	case KEY_F9:               res = USBKEY_F9;                break;
	case KEY_F10:              res = USBKEY_F10;               break;
	case KEY_NUMLOCK:          res = USBKEY_NUM_LOCK;          break;
	case KEY_SCROLLLOCK:       res = USBKEY_SCROLL_LOCK;       break;
	case KEY_KP7:              res = USBKEY_KP_7;              break;
	case KEY_KP8:              res = USBKEY_KP_8;              break;
	case KEY_KP9:              res = USBKEY_KP_9;              break;
	case KEY_KPMINUS:          res = USBKEY_KP_SUBTRACT;       break;
	case KEY_KP4:              res = USBKEY_KP_4;              break;
	case KEY_KP5:              res = USBKEY_KP_5;              break;
	case KEY_KP6:              res = USBKEY_KP_6;              break;
	case KEY_KPPLUS:           res = USBKEY_KP_ADD;            break;
	case KEY_KP1:              res = USBKEY_KP_1;              break;
	case KEY_KP2:              res = USBKEY_KP_2;              break;
	case KEY_KP3:              res = USBKEY_KP_3;              break;
	case KEY_KP0:              res = USBKEY_KP_0;              break;
	case KEY_KPDOT:            res = USBKEY_KP_DECIMAL;        break;
	case KEY_ZENKAKUHANKAKU:   res = USBKEY_LANG_5;            break;
	case KEY_102ND:            res = USBKEY_NON_US_BACKSLASH;  break;
	case KEY_F11:              res = USBKEY_F11;               break;
	case KEY_F12:              res = USBKEY_F12;               break;
	case KEY_RO:               res = USBKEY_INT_1;             break;
	case KEY_KATAKANA:         res = USBKEY_LANG_3;            break;
	case KEY_HIRAGANA:         res = USBKEY_LANG_4;            break;
	case KEY_HENKAN:           res = USBKEY_INT_4;             break;
	case KEY_KATAKANAHIRAGANA: res = USBKEY_INT_2;             break;
	case KEY_MUHENKAN:         res = USBKEY_INT_5;             break;
	case KEY_KPJPCOMMA:        res = USBKEY_INT_6;             break;
	case KEY_KPENTER:          res = USBKEY_KP_ENTER;          break;
	case KEY_RIGHTCTRL:        res = USBKEY_RIGHT_CONTROL;     break;
	case KEY_KPSLASH:          res = USBKEY_KP_DIVIDE;         break;
	case KEY_SYSRQ:            res = USBKEY_PRINT_SCREEN;      break;
	case KEY_RIGHTALT:         res = USBKEY_RIGHT_ALT;         break;
/*	case KEY_LINEFEED:         res = USBKEY_NO_EVENT;          break;*/
	case KEY_HOME:             res = USBKEY_HOME;              break;
	case KEY_UP:               res = USBKEY_UP_ARROW;          break;
	case KEY_PAGEUP:           res = USBKEY_PAGE_UP;           break;
	case KEY_LEFT:             res = USBKEY_LEFT_ARROW;        break;
	case KEY_RIGHT:            res = USBKEY_RIGHT_ARROW;       break;
	case KEY_END:              res = USBKEY_END;               break;
	case KEY_DOWN:             res = USBKEY_DOWN_ARROW;        break;
	case KEY_PAGEDOWN:         res = USBKEY_PAGE_DOWN;         break;
	case KEY_INSERT:           res = USBKEY_INSERT;            break;
	case KEY_DELETE:           res = USBKEY_DELETE;            break;
/*	case KEY_MACRO:            res = USBKEY_NO_EVENT;          break;*/
	case KEY_MUTE:             res = USBKEY_MUTE;              break;
	case KEY_VOLUMEDOWN:       res = USBKEY_VOLUME_DOWN;       break;
	case KEY_VOLUMEUP:         res = USBKEY_VOLUME_UP;         break;
	case KEY_POWER:            res = USBKEY_POWER;             break;
	case KEY_KPEQUAL:          res = USBKEY_KP_EQUAL;          break;
	case KEY_KPPLUSMINUS:      res = USBKEY_KP_PLUS_MINUS;     break;
	case KEY_PAUSE:            res = USBKEY_PAUSE;             break;
/*	case KEY_SCALE:            res = USBKEY_NO_EVENT;          break;*/
	case KEY_KPCOMMA:          res = USBKEY_KP_COMMA;          break;
	case KEY_HANGEUL:          res = USBKEY_LANG_1;            break;
	case KEY_HANJA:            res = USBKEY_LANG_2;            break;
	case KEY_YEN:              res = USBKEY_INT_3;             break;
	case KEY_LEFTMETA:         res = USBKEY_LEFT_GUI;          break;
	case KEY_RIGHTMETA:        res = USBKEY_RIGHT_GUI;         break;
	case KEY_COMPOSE:          res = USBKEY_APPLICATION;       break;
	case KEY_STOP:             res = USBKEY_STOP;              break;
	case KEY_AGAIN:            res = USBKEY_AGAIN;             break;
	case KEY_PROPS:            res = USBKEY_MENU;              break;
	case KEY_UNDO:             res = USBKEY_UNDO;              break;
	case KEY_FRONT:            res = USBKEY_SELECT;            break;
	case KEY_COPY:             res = USBKEY_COPY;              break;
	case KEY_OPEN:             res = USBKEY_EXECUTE;           break;
	case KEY_PASTE:            res = USBKEY_PASTE;             break;
	case KEY_FIND:             res = USBKEY_FIND;              break;
	case KEY_CUT:              res = USBKEY_CUT;               break;
	case KEY_HELP:             res = USBKEY_HELP;              break;
/*	case KEY_MENU:             res = USBKEY_NO_EVENT;          break;
	case KEY_CALC:             res = USBKEY_NO_EVENT;          break;
	case KEY_SETUP:            res = USBKEY_NO_EVENT;          break;
	case KEY_SLEEP:            res = USBKEY_NO_EVENT;          break;
	case KEY_WAKEUP:           res = USBKEY_NO_EVENT;          break;
	case KEY_FILE:             res = USBKEY_NO_EVENT;          break;
	case KEY_SENDFILE:         res = USBKEY_NO_EVENT;          break;
	case KEY_DELETEFILE:       res = USBKEY_NO_EVENT;          break;
	case KEY_XFER:             res = USBKEY_NO_EVENT;          break;
	case KEY_PROG1:            res = USBKEY_NO_EVENT;          break;
	case KEY_PROG2:            res = USBKEY_NO_EVENT;          break;
	case KEY_WWW:              res = USBKEY_NO_EVENT;          break;
	case KEY_MSDOS:            res = USBKEY_NO_EVENT;          break;
	case KEY_SCREENLOCK:       res = USBKEY_NO_EVENT;          break;
	case KEY_ROTATE_DISPLAY:   res = USBKEY_NO_EVENT;          break;
	case KEY_CYCLEWINDOWS:     res = USBKEY_NO_EVENT;          break;
	case KEY_MAIL:             res = USBKEY_NO_EVENT;          break;
	case KEY_BOOKMARKS:        res = USBKEY_NO_EVENT;          break;
	case KEY_COMPUTER:         res = USBKEY_NO_EVENT;          break;
	case KEY_BACK:             res = USBKEY_NO_EVENT;          break;
	case KEY_FORWARD:          res = USBKEY_NO_EVENT;          break;
	case KEY_CLOSECD:          res = USBKEY_NO_EVENT;          break;
	case KEY_EJECTCD:          res = USBKEY_NO_EVENT;          break;
	case KEY_EJECTCLOSECD:     res = USBKEY_NO_EVENT;          break;
	case KEY_NEXTSONG:         res = USBKEY_NO_EVENT;          break;
	case KEY_PLAYPAUSE:        res = USBKEY_NO_EVENT;          break;
	case KEY_PREVIOUSSONG:     res = USBKEY_NO_EVENT;          break;
	case KEY_STOPCD:           res = USBKEY_NO_EVENT;          break;
	case KEY_RECORD:           res = USBKEY_NO_EVENT;          break;
	case KEY_REWIND:           res = USBKEY_NO_EVENT;          break;
	case KEY_PHONE:            res = USBKEY_NO_EVENT;          break;
	case KEY_ISO:              res = USBKEY_NO_EVENT;          break;
	case KEY_CONFIG:           res = USBKEY_NO_EVENT;          break;
	case KEY_HOMEPAGE:         res = USBKEY_NO_EVENT;          break;
	case KEY_REFRESH:          res = USBKEY_NO_EVENT;          break;
	case KEY_EXIT:             res = USBKEY_NO_EVENT;          break;
	case KEY_MOVE:             res = USBKEY_NO_EVENT;          break;
	case KEY_EDIT:             res = USBKEY_NO_EVENT;          break;
	case KEY_SCROLLUP:         res = USBKEY_NO_EVENT;          break;
	case KEY_SCROLLDOWN:       res = USBKEY_NO_EVENT;          break;
	case KEY_KPLEFTPAREN:      res = USBKEY_NO_EVENT;          break;
	case KEY_KPRIGHTPAREN:     res = USBKEY_NO_EVENT;          break;
	case KEY_NEW:              res = USBKEY_NO_EVENT;          break;
	case KEY_REDO:             res = USBKEY_NO_EVENT;          break;*/
	case KEY_F13:              res = USBKEY_F13;               break;
	case KEY_F14:              res = USBKEY_F14;               break;
	case KEY_F15:              res = USBKEY_F15;               break;
	case KEY_F16:              res = USBKEY_F16;               break;
	case KEY_F17:              res = USBKEY_F17;               break;
	case KEY_F18:              res = USBKEY_F18;               break;
	case KEY_F19:              res = USBKEY_F19;               break;
	case KEY_F20:              res = USBKEY_F20;               break;
	case KEY_F21:              res = USBKEY_F21;               break;
	case KEY_F22:              res = USBKEY_F22;               break;
	case KEY_F23:              res = USBKEY_F23;               break;
	case KEY_F24:              res = USBKEY_F24;               break;
/*	case KEY_PLAYCD:           res = USBKEY_NO_EVENT;          break;
	case KEY_PAUSECD:          res = USBKEY_NO_EVENT;          break;
	case KEY_PROG3:            res = USBKEY_NO_EVENT;          break;
	case KEY_PROG4:            res = USBKEY_NO_EVENT;          break;
	case KEY_ALL_APPLICATIONS: res = USBKEY_NO_EVENT;          break;
	case KEY_SUSPEND:          res = USBKEY_NO_EVENT;          break;
	case KEY_CLOSE:            res = USBKEY_NO_EVENT;          break;
	case KEY_PLAY:             res = USBKEY_NO_EVENT;          break;
	case KEY_FASTFORWARD:      res = USBKEY_NO_EVENT;          break;
	case KEY_BASSBOOST:        res = USBKEY_NO_EVENT;          break;
	case KEY_PRINT:            res = USBKEY_NO_EVENT;          break;
	case KEY_HP:               res = USBKEY_NO_EVENT;          break;
	case KEY_CAMERA:           res = USBKEY_NO_EVENT;          break;
	case KEY_SOUND:            res = USBKEY_NO_EVENT;          break;
	case KEY_QUESTION:         res = USBKEY_NO_EVENT;          break;
	case KEY_EMAIL:            res = USBKEY_NO_EVENT;          break;
	case KEY_CHAT:             res = USBKEY_NO_EVENT;          break;
	case KEY_SEARCH:           res = USBKEY_NO_EVENT;          break;
	case KEY_CONNECT:          res = USBKEY_NO_EVENT;          break;
	case KEY_FINANCE:          res = USBKEY_NO_EVENT;          break;
	case KEY_SPORT:            res = USBKEY_NO_EVENT;          break;
	case KEY_SHOP:             res = USBKEY_NO_EVENT;          break;
	case KEY_ALTERASE:         res = USBKEY_NO_EVENT;          break;
	case KEY_CANCEL:           res = USBKEY_NO_EVENT;          break;
	case KEY_BRIGHTNESSDOWN:   res = USBKEY_NO_EVENT;          break;
	case KEY_BRIGHTNESSUP:     res = USBKEY_NO_EVENT;          break;
	case KEY_MEDIA:            res = USBKEY_NO_EVENT;          break;
	case KEY_SWITCHVIDEOMODE:  res = USBKEY_NO_EVENT;          break;
	case KEY_KBDILLUMTOGGLE:   res = USBKEY_NO_EVENT;          break;
	case KEY_KBDILLUMDOWN:     res = USBKEY_NO_EVENT;          break;
	case KEY_KBDILLUMUP:       res = USBKEY_NO_EVENT;          break;
	case KEY_SEND:             res = USBKEY_NO_EVENT;          break;
	case KEY_REPLY:            res = USBKEY_NO_EVENT;          break;
	case KEY_FORWARDMAIL:      res = USBKEY_NO_EVENT;          break;
	case KEY_SAVE:             res = USBKEY_NO_EVENT;          break;
	case KEY_DOCUMENTS:        res = USBKEY_NO_EVENT;          break;
	case KEY_BATTERY:          res = USBKEY_NO_EVENT;          break;
	case KEY_BLUETOOTH:        res = USBKEY_NO_EVENT;          break;
	case KEY_WLAN:             res = USBKEY_NO_EVENT;          break;
	case KEY_UWB:              res = USBKEY_NO_EVENT;          break;
	case KEY_UNKNOWN:          res = USBKEY_NO_EVENT;          break;
	case KEY_VIDEO_NEXT:       res = USBKEY_NO_EVENT;          break;
	case KEY_VIDEO_PREV:       res = USBKEY_NO_EVENT;          break;
	case KEY_BRIGHTNESS_CYCLE: res = USBKEY_NO_EVENT;          break;
	case KEY_BRIGHTNESS_AUTO:  res = USBKEY_NO_EVENT;          break;
	case KEY_DISPLAY_OFF:      res = USBKEY_NO_EVENT;          break;
	case KEY_WWAN:             res = USBKEY_NO_EVENT;          break;
	case KEY_RFKILL:           res = USBKEY_NO_EVENT;          break;
	case KEY_MICMUTE:          res = USBKEY_NO_EVENT;          break;
	default:                   res = USBKEY_NO_EVENT;          break;*/
	}
	return res;
}


/**
 * Clamps one integer type to the range of another one.
 *
 * @param[in] val - input value
 * @return clamped output value
 */
template <typename R, typename T>
static inline R clamp(const T val) noexcept {
	return R((val > std::numeric_limits<R>::max()) ? std::numeric_limits<R>::max() : (val < std::numeric_limits<R>::min()) ? std::numeric_limits<R>::min() : val);
}


/**
 * Tests if a bit within the given array is set or not.
 *
 * @param[in] bit - bit index to test
 * @param[in] array - bit array
 * @return true if set, else false
 */
static inline bool testBit(const uint32_t bit, uint8_t * array) noexcept {
	return bool(array[bit / 8] & (1 << (bit & 7)));
}


/**
 * libinput specific callback function to handle device open operations.
 *
 * @param[in] path - path to the device
 * @param[in] flags - POSIX `open()` flags
 * @param[in] userData - user specific data (`InputDevice *` object)
 * @return file descriptor or negative error number
 */
static int libInputOpen(const char * path, int /* flags */, void * userData){
	if (userData == NULL) return -EFAULT;
	InputDevice * dev = static_cast<InputDevice *>(userData);
	errno = 0;
	dev->fd = ::open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (dev->fd < 0) return -errno;
	return dev->fd;
}

/**
 * libinput specific callback function to close a file descriptor.
 *
 * @param[in] fd - file descriptor
 * @param[in] userData - user specific data
 */
static void libInputClose(int fd, void * /* userData */){
	::close(fd);
}


/**
 * Function table for libinput.
 */
static const struct libinput_interface libInputInterface = {
	libInputOpen,
	libInputClose,
};


#endif /* PCF_IS_LINUX */


/**
 * Starts or stops capturing of keyboard/mouse events globally.
 * No events are forwarded to other applications and windows until
 * capturing has been stopped.
 *
 * @param[in] enable - set true to enable capturing, else false
 * @return true on success, else false
 */
bool VkvmDevice::grabGlobalInput(const bool enable) {
	static std::mutex mutex;
	std::unique_lock<std::mutex> fnGuard(mutex); /* only one thread may modify this global state at a time */
	if (self->grabbingInput == enable) return false; /* no change requested */
	if (enable && ( ! this->isConnected() )) return false; /* not connected */
#if defined(PCF_IS_WIN)
	if ( enable ) {
		if ( ! this->isConnected() ) return false;
		self->common.hasLastMouse = false;
		/* ensure proper keyboard/mouse states */
		this->keyboardAllUp();
		this->mouseButtonAllUp();
		/* install hook */
		self->common.hookProcState = SerialCommon::HPS_START;
		self->common.hookProcWait.lock();
		self->common.hookProcThread = std::thread([this] () {
			try {
				vkvmHookCtx = &(self->common);
				const auto resetCtxOnReturn = makeScopeExit([&]() { vkvmHookCtx = NULL; });
				HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
				self->common.hookProcThreadId = GetCurrentThreadId();
				self->common.keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, lowLevelKeyboardProc, hInstance, 0);
				if (self->common.keyboardHook == NULL) {
					self->common.hookProcState = SerialCommon::HPS_FAILED;
					self->common.hookProcWait.unlock();
					return;
				}
				self->common.mouseHook = SetWindowsHookEx(WH_MOUSE_LL, lowLevelMouseProc, hInstance, 0);
				if (self->common.mouseHook == NULL) {
					UnhookWindowsHookEx(self->common.keyboardHook);
					self->common.hookProcState = SerialCommon::HPS_FAILED;
					self->common.hookProcWait.unlock();
					return;
				}
				self->common.hookProcState = SerialCommon::HPS_IDLE;
				self->common.hookProcWait.unlock();
				MSG msg;
				while (self->common.hookProcState == SerialCommon::HPS_IDLE && GetMessage(&msg, NULL, 0, 0) > 0) {
						TranslateMessage(&msg);
						DispatchMessage(&msg);
				}
				/* uninstall hooks */
				UnhookWindowsHookEx(self->common.keyboardHook);
				UnhookWindowsHookEx(self->common.mouseHook);
				/* reset keyboard/mouse states */
				self->common.device->keyboardAllUp();
				self->common.device->mouseButtonAllUp();
			} catch (...) {}
		});
		self->common.hookProcWait.lock(); /* wait for startup */
		self->common.hookProcWait.unlock();
		if (self->common.hookProcState == SerialCommon::HPS_FAILED) {
			return false;
		}
	} else {
		/* terminate hook processing thread */
		if ( self->common.hookProcThread.joinable() ) {
			PostThreadMessage(self->common.hookProcThreadId, WM_QUIT, 0, 0);
			self->common.hookProcThread.join();
		}
		/* release meta keys to avoid them remain being pressed */
		INPUT simInput;
		const WORD keys[] = {
			VK_LSHIFT, VK_RSHIFT,
			VK_LCONTROL, VK_RCONTROL,
			VK_LMENU, VK_RMENU,
			VK_LWIN, VK_RWIN
		};
		simInput.type = INPUT_KEYBOARD;
		simInput.ki.dwFlags = KEYEVENTF_KEYUP;
		simInput.ki.time = 0;
		simInput.ki.dwExtraInfo = ULONG_PTR(GetMessageExtraInfo());
		for (size_t n = 0; n < (sizeof(keys) / sizeof(*keys)); n++) {
			/* reset meta key state */
			simInput.ki.wVk = keys[n];
			SendInput(1, &simInput, sizeof(simInput));
		}
		/* recover lost keyboard states */
		const WORD stateKeys[] = {
			VK_NUMLOCK,
			VK_CAPITAL,
			VK_SCROLL,
			VK_KANA
		};
		for (size_t n = 0; n < (sizeof(stateKeys) / sizeof(*stateKeys)); n++) {
			simInput.ki.wVk = stateKeys[n];
			for (size_t k = 0; k < 2; k++) {
				simInput.ki.dwFlags = 0;
				SendInput(1, &simInput, sizeof(simInput));
				simInput.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, &simInput, sizeof(simInput));
			}
		}
	}
#elif defined(PCF_IS_LINUX)
	/* @see https://www.kernel.org/doc/Documentation/input/input.txt */
	if ( enable ) {
		if ( ! this->isConnected() ) return false;
		if (self->common.hookTermFd >= 0) return false; /* still grabbing */
		self->common.hookTermFd = eventfd(0, EFD_NONBLOCK);
		if (self->common.hookTermFd < 0) return false;
		/* ensure proper keyboard/mouse states */
		this->keyboardAllUp();
		this->mouseButtonAllUp();
		/* install hook */
		self->common.hookProcState = SerialCommon::HPS_START;
		self->common.hookProcWait.lock();
		self->common.hookProcThread = std::thread([this] () {
			try {
				struct dirent ** dirList;
				bool succeeded = false; /* startup succeeded? */
				const auto reportOnReturn = makeScopeExit([this, &succeeded]() {
					if ( ! succeeded ) {
						self->common.hookProcState = SerialCommon::HPS_FAILED;
						self->common.hookProcWait.unlock();
					}
				});
				/* initialize libinput context */
				struct libinput * li = libinput_path_create_context(&libInputInterface, NULL);
				const auto freeLibinputOnReturn = makeScopeExit([=]() {
					if (li != NULL) libinput_unref(li);
				});
				/* open all input devices; obtain actual device path from sysfs */
				const int count = xEINTR(scandir, "/sys/class/input", &dirList, nullptr, nullptr);
				if (count <= 0) return;
				const auto freeDirListOnReturn = makeScopeExit([=]() {
					for (int n = 0; n < count; n++) free(dirList[n]);
					free(dirList);
				});
				int devCount = 0;
				InputDevice * devList = static_cast<InputDevice *>(malloc(size_t(count) * sizeof(InputDevice)));
				if (devList == NULL) return;
				for (int n = 0; n < count; n++) new (devList + n) InputDevice();
				const auto freeDevListOnReturn = makeScopeExit([=, &devCount]() {
					for (int n = 0; n < devCount; n++) devList[n].~InputDevice();
					free(devList);
				});
				char * buffers = static_cast<char *>(malloc((PCF_MAX_SYS_PATH + PCF_MAX_UEVENT_LINE) * sizeof(char)));
				if (buffers == NULL) return;
				char * buffer0 = buffers;
				char * buffer1 = buffers + PCF_MAX_SYS_PATH;
				const auto freeBuffersOnReturn = makeScopeExit([=]() { free(buffers); });
				for (int n = 0; n < count; n++) {
					if (strcmp(dirList[n]->d_name, ".") == 0) continue;
					if (strcmp(dirList[n]->d_name, "..") == 0) continue;
					if (strncmp("event", dirList[n]->d_name, 5) != 0) continue; /* use only "event" devices */
					if (snprintf(buffer0, PCF_MAX_SYS_PATH, "/sys/class/input/%s/uevent", dirList[n]->d_name) < 1) continue; /* path too long */
					/* get actual device path */
					FILE * fd = fopen(buffer0, "r");
					if (fd == NULL) continue;
					const auto closeFdOnNext = makeScopeExit([=]() { fclose(fd); });
					while (fgets(buffer1, PCF_MAX_UEVENT_LINE, fd) != NULL) {
						buffer1[PCF_MAX_UEVENT_LINE - 1] = 0;
						if (strncmp("DEVNAME=", buffer1, 8) == 0) {
							char * eol = strchr(buffer1 + 8, '\n');
							if (eol != NULL) *eol = 0;
							if (snprintf(buffer0, PCF_MAX_SYS_PATH, "/dev/%s", buffer1 + 8) < 1) break; /* path too long */
							errno = 0;
							libinput_set_user_data(li, devList + devCount);
							/* see `libInputOpen()` */
							devList[devCount].liDev = libinput_path_add_device(li, buffer0);
							libinput_set_user_data(li, NULL);
							if (devList[devCount].liDev == NULL) {
								vkvmTrace(3, "error\topen\t%s\t%s\n", buffer0, strerror(errno));
								continue;
							}
							libinput_device_ref(devList[devCount].liDev);
							const size_t len = strlen(buffer0) + 1;
							devList[devCount].path = static_cast<char *>(malloc(len * sizeof(char)));
							if (devList[devCount].path != NULL) memcpy(devList[devCount].path, buffer0, len * sizeof(char));
							devCount++;
							break;
						}
					}
				}
				if (devCount <= 0) return; /* no input devices found */
				const auto releaseRemoteKeysOnReturn = makeScopeExit([=]() {
					/* reset keyboard/mouse states */
					self->common.device->keyboardAllUp();
					self->common.device->mouseButtonAllUp();
				});
				const auto tryGrabInputs = [this, &succeeded] (InputDevice & dev) -> bool {
					if ( dev.grabbed ) return true; /* already grabbing */
					if ( dev.failed ) return false;
					/* get bitmap of key press states */
					uint8_t keyBits[(KEY_MAX / 8) + 1];
					memset(keyBits, 0, sizeof(keyBits));
					errno = 0;
					if (xEINTR(ioctl, dev.fd, EVIOCGKEY(sizeof(keyBits)), keyBits) < 0) {
						dev.failed = true;
						vkvmTrace(3, "error\tgetKeyBits\t%s\t%s\n", dev.path, strerror(errno));
						return false;
					}
					/* check if all keys have been released */
					for (int n = 0; n < int(sizeof(keyBits)); n++) {
						if (keyBits[n] != 0) return false; /* some keys are pressed */
					}
					/* all keys have been released -> start grabbing input */
					errno = 0;
					if (xEINTR(ioctl, dev.fd, EVIOCGRAB, GRAB_ON) < 0) { /* grab */
						dev.failed = true;
						vkvmTrace(3, "error\tgrab\t%s\t%s\n", dev.path, strerror(errno));
						return false;
					}
					vkvmTrace(3, "grab\t%s\n", dev.path);
					dev.grabbed = true;
					if ( ! succeeded ) {
						/* notify `grabGlobalInput()` that at least one device input has been grabbed */
						self->common.hookProcState = SerialCommon::HPS_IDLE;
						self->common.hookProcWait.unlock();
						succeeded = true;
					}
					return true;
				};
				/* start grabbing after all keys have been released */
				fd_set readFds;
				struct timeval tout;
				/* main event loop (grab input as soon as possible) */
				for ( ;; ) {
					bool hasDevice = false;
					for (int n = 0; n < devCount; n++) {
						InputDevice & dev = devList[n];
						tryGrabInputs(dev);
						if ( ! dev.failed ) hasDevice = true;
					}
					if ( ! hasDevice ) return; /* failed grabbing for all input devices */
					FD_ZERO(&readFds);
					FD_SET(self->common.hookTermFd, &readFds);
					FD_SET(libinput_get_fd(li), &readFds);
					const int maxFd = std::max(self->common.hookTermFd, libinput_get_fd(li));
					/* wait for next key event */
					tout.tv_sec = 0;
					tout.tv_usec = succeeded ? 500000 : 250000;
					errno = 0;
					const int sRes = select(maxFd + 1, &readFds, NULL, NULL, &tout);
					if (sRes < 0) {
						if (errno == EAGAIN || errno == EINTR) continue;
						return;
					}
					if ( FD_ISSET(self->common.hookTermFd, &readFds) ) return;
					/* read input events */
					if (libinput_dispatch(li) < 0) continue;
					/* accumulate mouse movement to compensate transmission speed */
					int16_t absX, absY;
					bool hasAbsXY = false;
					InputDevice::ValueType relX(0), relY(0), relWheel(0);
					struct libinput_event * event;
					while ((event = libinput_get_event(li)) != NULL) {
						switch (libinput_event_get_type(event)) {
						case LIBINPUT_EVENT_KEYBOARD_KEY: {
							struct libinput_event_keyboard * keyEvent = libinput_event_get_keyboard_event(event);
							const uint32_t keyCode = libinput_event_keyboard_get_key(keyEvent);
							switch (libinput_event_keyboard_get_key_state(keyEvent)) {
							case LIBINPUT_KEY_STATE_PRESSED:
								self->common.device->keyboardDown(mapKeyCode(keyCode), keyCode);
								break;
							case LIBINPUT_KEY_STATE_RELEASED:
								self->common.device->keyboardUp(mapKeyCode(keyCode), keyCode);
								break;
							}
							} break;
						case LIBINPUT_EVENT_POINTER_BUTTON: {
							struct libinput_event_pointer * pointerEvent = libinput_event_get_pointer_event(event);
							switch (libinput_event_pointer_get_button_state(pointerEvent)) {
							case LIBINPUT_BUTTON_STATE_PRESSED:
								switch (libinput_event_pointer_get_button(pointerEvent)) {
								case BTN_LEFT:   self->common.device->mouseButtonDown(USBBUTTON_LEFT);   break;
								case BTN_RIGHT:  self->common.device->mouseButtonDown(USBBUTTON_RIGHT);  break;
								case BTN_MIDDLE: self->common.device->mouseButtonDown(USBBUTTON_MIDDLE); break;
								default: break;
								}
								break;
							case LIBINPUT_BUTTON_STATE_RELEASED:
								switch (libinput_event_pointer_get_button(pointerEvent)) {
								case BTN_LEFT:   self->common.device->mouseButtonUp(USBBUTTON_LEFT);   break;
								case BTN_RIGHT:  self->common.device->mouseButtonUp(USBBUTTON_RIGHT);  break;
								case BTN_MIDDLE: self->common.device->mouseButtonUp(USBBUTTON_MIDDLE); break;
								default: break;
								}
								break;
							}
							} break;
						case LIBINPUT_EVENT_POINTER_MOTION: {
							struct libinput_event_pointer * pointerEvent = libinput_event_get_pointer_event(event);
							relX += InputDevice::round(libinput_event_pointer_get_dx(pointerEvent));
							relY += InputDevice::round(libinput_event_pointer_get_dy(pointerEvent));
							} break;
						case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE: {
							struct libinput_event_pointer * pointerEvent = libinput_event_get_pointer_event(event);
							absX = InputDevice::roundAbs(libinput_event_pointer_get_absolute_x_transformed(pointerEvent, 32768));
							absY = InputDevice::roundAbs(libinput_event_pointer_get_absolute_y_transformed(pointerEvent, 32768));
							hasAbsXY = true;
							} break;
						case LIBINPUT_EVENT_POINTER_AXIS: {
							struct libinput_event_pointer * pointerEvent = libinput_event_get_pointer_event(event);
							switch (libinput_event_pointer_get_axis_source(pointerEvent)) {
							case LIBINPUT_POINTER_AXIS_SOURCE_WHEEL:
								relWheel += InputDevice::round(-libinput_event_pointer_get_axis_value_discrete(pointerEvent, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL));
								break;
							case LIBINPUT_POINTER_AXIS_SOURCE_FINGER:
								relWheel += InputDevice::round(-libinput_event_pointer_get_axis_value(pointerEvent, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL) / 3.0);
								break;
							default: break;
							}
							} break;
						default: break;
						}
						/* get next event */
						libinput_event_destroy(event);
						libinput_dispatch(li);
					}
					if ( hasAbsXY ) {
						self->common.device->mouseMoveAbs(absX, absY);
					}
					/* update mouse/wheel movements from accumulated values */
					while (relX != 0 || relY != 0) {
						const InputDevice::ValueType moveX = PCF_MIN(PCF_MAX(relX, -127), 127);
						const InputDevice::ValueType moveY = PCF_MIN(PCF_MAX(relY, -127), 127);
						if ( ! self->common.device->mouseMoveRel(int8_t(moveX), int8_t(moveY)) ) break;
						relX -= moveX;
						relY -= moveY;
					}
					while (relWheel != 0) {
						const InputDevice::ValueType moveWheel = PCF_MIN(PCF_MAX(relWheel, -127), 127);
						if ( ! self->common.device->mouseScroll(int8_t(moveWheel)) ) break;
						relWheel -= moveWheel;
					}
				}
			} catch (...) {}
		});
		self->common.hookProcWait.lock(); /* wait for startup */
		self->common.hookProcWait.unlock();
		if (self->common.hookProcState == SerialCommon::HPS_FAILED) {
			if ( self->common.hookProcThread.joinable() ) {
				self->common.hookProcThread.join();
			}
			::close(self->common.hookTermFd);
			self->common.hookTermFd = -1;
			return false;
		}
	} else {
		/* terminate hook processing thread */
		std::unique_lock<std::mutex> guard(self->common.hookProcWait);
		if ( self->common.hookProcThread.joinable() ) {
			if (self->common.hookTermFd >= 0) {
				uint64_t term = 1;
				while (xEINTR(write, self->common.hookTermFd, &term, sizeof(term)) != ssize_t(sizeof(term))) usleep(10000);
			}
			self->common.hookProcThread.join();
			if (self->common.hookTermFd >= 0) {
				::close(self->common.hookTermFd);
				self->common.hookTermFd = -1;
			}
		}
	}
#endif /* PCF_IS_LINUX */
	self->grabbingInput = enable;
	return true;
}


} /* namespace serial */
} /* namespace pcf */
