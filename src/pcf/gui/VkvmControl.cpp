/**
 * @file VkvmControl.cpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2023-11-25
 *
 * @todo reconnect last capture/serial device if temporary lost (with old settings)
 */
#include <algorithm>
#include <condition_variable>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <FL/fl_ask.H>
#include <FL/Fl_Text_Display.H>
#include <libpcf/natcmps.h>
#include <pcf/gui/VkvmControl.hpp>
#include <pcf/gui/SvgData.hpp>
#include <pcf/ScopeExit.hpp>
#include <pcf/Utility.hpp>
#include <license.hpp>


#define SERIAL_COLOR_FULLY_CONNECTED Fl::get_color(179)
#define SERIAL_COLOR_CONNECTED Fl::get_color(60)
#define SERIAL_COLOR_PENDING Fl::get_color(91)
#define SERIAL_COLOR_DISCONNECTED Fl::get_color(130)
#define SERIAL_COLOR_PASTE_COMPLETE FL_FOREGROUND_COLOR
#define SERIAL_COLOR_PASTE_PENDING Fl::get_color(91)
#define STATUS_COLOR_LED_OFF FL_FOREGROUND_COLOR
#define STATUS_COLOR_LED_ON Fl::get_color(60)
#define SERIAL_SPEED 115200
#define SERIAL_FRAMING SFR_8N1
#define SERIAL_FLOW SFC_NONE


namespace pcf {
namespace gui {


namespace {
/**
 * License information window.
 */
class LicenseInfoWindow : public Fl_Double_Window {
private:
	Fl_Text_Buffer * buffer;
	Fl_Text_Display * license;
	SvgButton * ok;
public:
	explicit LicenseInfoWindow(const int W, const int H, const char * L = NULL):
		Fl_Double_Window(W, H, L),
		buffer(NULL),
		license(NULL),
		ok(NULL)
	{
		const int spaceH = adjDpiH(10);
		const int spaceV = adjDpiV(10);
		const int widgetV = adjDpiV(26);
		const int licenseV = H - (3 * spaceV) - (2 * widgetV);
		int y1 = spaceV;

		buffer = new Fl_Text_Buffer();
		license = new Fl_Text_Display(spaceH, y1 + widgetV, W - (2 * spaceH), licenseV, "License");
		buffer->text(licenseText);
		license->buffer(buffer);
		license->hide_cursor();
		license->textfont(FL_COURIER);
		license->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
		license->box(FL_BORDER_BOX);
		license->textsize(8 * FL_NORMAL_SIZE / 10);
		license->scroll(0, 0); /* update scroll range according to content */
		y1 = license->y() + license->h() + spaceV;

		ok = new SvgButton((W - widgetV) / 2, y1, widgetV, widgetV, okSvg);
		ok->colorButton(true);
		ok->labelcolor(FL_FOREGROUND_COLOR);
		ok->callback(PCF_GUI_CALLBACK(onOk), this);

		resizable(license);
		end();
	}

	virtual ~LicenseInfoWindow() {
		delete this->ok;
		delete this->license;
		delete this->buffer;
	}

private:
	PCF_GUI_BIND(LicenseInfoWindow, onOk, SvgButton);

	inline void onOk(SvgButton * /* button */) {
		this->hide();
	}
};


/**
 * Helper class to implement a mouse push callback handle on Fl_Box.
 */
class OnClickBox : public Fl_Box {
public:
	/**
	 * Constructor.
	 *
	 * @param[in] X - x coordinate
	 * @param[in] Y - y coordinate
	 * @param[in] W - width
	 * @param[in] H - height
	 * @param[in] L - label (optional)
	 */
	explicit inline OnClickBox(const int X, const int Y, const int W, const int H, const char * L = NULL):
		Fl_Box(X, Y, W, H, L)
	{}

	/** Destructor */
	virtual ~OnClickBox() {}

	/**
	 * Overwritten event handler.
	 *
	 * @param[in] e - event
	 * @return 0 - if the event was not used or understood
	 * @return 1 - if the event was used and can be deleted
	 */
	inline int handle(int e) {
		int result = Fl_Box::handle(e);
		switch (e) {
		case FL_PUSH:
			do_callback();
			break;
		default:
			break;
		}
		return result;
	}
};


/**
 * Returns the SVG which corresponds to the given rotation value.
 *
 * @param[in] val - rotation value
 * @return associated SVG data
 */
static inline const char * getRotationSvg(const VkvmView::Rotation val) {
	switch (val) {
	case VkvmView::ROT_RIGHT: return rightSvg;
	case VkvmView::ROT_DOWN:  return downSvg;
	case VkvmView::ROT_LEFT:  return leftSvg;
	case VkvmView::ROT_UP:    return upSvg;
	default:                  return upSvg;
	}
}


/**
 * Helper structure to hold a status history item.
 */
struct StatusItem {
	enum Flag {
		IS_UNSET = 0x00,
		IS_SET = 0x01,
		IS_ALLOCATED = 0x02
	};

	time_t dateTime;
	const char * text;
	unsigned int flags;

	explicit inline StatusItem():
		text(NULL),
		flags(IS_UNSET)
	{}

	explicit inline StatusItem(const time_t dt, const char * l = NULL, const bool a = false):
		dateTime(dt),
		text(a ? NULL : l),
		flags(IS_SET)
	{
		if ( a ) this->copyLabel(l);
	}

	inline StatusItem(const StatusItem & o):
		dateTime(o.dateTime),
		text(((o.flags & IS_ALLOCATED) != 0) ? NULL : o.text),
		flags(o.flags & IS_SET)
	{
		if ((o.flags & IS_ALLOCATED) != 0) this->copyLabel(o.text);
	}

	inline StatusItem(StatusItem && o):
		dateTime(o.dateTime),
		text(o.text),
		flags(o.flags)
	{
		o.text = NULL;
		o.flags = IS_UNSET;
	}

	inline ~StatusItem() {
		if (this->text != NULL && (this->flags & IS_ALLOCATED) != 0) {
			free(const_cast<char *>(this->text));
		}
	}

	inline StatusItem & operator= (const StatusItem & o) {
		if (*this != o) {
			this->dateTime = o.dateTime;
			this->text = ((o.flags & IS_ALLOCATED) != 0) ? NULL : o.text;
			this->flags = o.flags & IS_SET;
			if ((o.flags & IS_ALLOCATED) != 0) this->copyLabel(o.text);
		}
		return *this;
	}

	inline StatusItem & operator= (StatusItem && o) {
		if (*this != o) {
			this->dateTime = o.dateTime;
			this->text = o.text;
			this->flags = o.flags;
			o.text = NULL;
			o.flags = IS_UNSET;
		}
		return *this;
	}

	inline operator bool() const {
		return (this->flags & IS_SET) != 0;
	}

	void copyLabel(const char * l) {
		if (this->text != NULL && (this->flags & IS_ALLOCATED) != 0) {
			free(const_cast<char *>(this->text));
			this->text = NULL;
		}
		if (l == NULL) return;
		const size_t len = strlen(l) + 1;
		char * t = static_cast<char *>(malloc(len * sizeof(char)));
		if (t == NULL) return;
		memcpy(t, l, len);
		this->text = t;
		this->flags = this->flags | IS_ALLOCATED;
	}
};


} /* anonymous namespace */


/**
 * Background serial VKVM device send data handling class.
 */
class VkvmControlSerialSend {
public:
	/** Possible send types. */
	enum SendType {
		SEND_ALT_CODE, /**< Send as ALT code with leading zero. */
		SEND_ALT_X, /**< Send as ALT-X code. */
		SEND_HEX_NUMPAD, /**< Send as hex numpad code. */
		SEND_ISO14755_HOLDING, /**< Send as GTK+ (ISO/IEC 14755) code holding CTRL-SHIFT. */
		SEND_ISO14755_HOLD_RELEASE, /**< Send as GTK+ (ISO/IEC 14755) code hold/release CTRL-SHIFT. */
		SEND_VI /**< Send as Vi/Vim code. */
	};
	/** Helper enumerations. */
	enum {
		ALL_KEYS = USBWRITE_LEFT_CONTROL | USBWRITE_LEFT_SHIFT | USBWRITE_LEFT_ALT | USBWRITE_RIGHT_CONTROL | USBWRITE_RIGHT_SHIFT | USBWRITE_RIGHT_ALT,
		ALL_LEDS = USBWRITE_RIGHT_NUM_LOCK | USBWRITE_RIGHT_KANA,
		NUMLOCK_LED = USBWRITE_RIGHT_NUM_LOCK
	};
private:
	pcf::serial::VkvmDevice * serialDevice; /**< Serial VKVM device instance to send to. */
	mutable std::mutex sendToMutex; /**< Limit parallel sendTo calls to one. */
	mutable std::mutex terminateMutex; /**< Termination variable mutex. */
	mutable std::condition_variable terminateSignal; /**< Termination signal handle. */
	mutable volatile bool running; /**< Background thread is running. */
	volatile bool terminate; /**< Termination request variable. */
	mutable volatile bool restarting; /**< Termination request due to sendTo() call. */
	std::thread sendThread; /**< Background thread instance. */
	char * str; /**< Data to send. */
	size_t len; /**< Length of the data to send. */
	Fl_Callback * cbFn; /**< Callback function. */
	Fl_Widget * cbWidget; /**< Associated callback widget. */
	void * cbArg; /**< Associated callback user argument. */
public:
	/** Constructor. */
	explicit inline VkvmControlSerialSend():
		serialDevice(NULL),
		running(false),
		terminate(false),
		restarting(false),
		str(NULL),
		cbFn(NULL),
		cbWidget(NULL),
		cbArg(NULL)
	{}

	/** Destructor. */
	inline ~VkvmControlSerialSend() {
		this->stop();
	}

	/**
	 * Sends the given string to the passed device in specified send type.
	 *
	 * @param[in,out] device - serial VKVM device to send to
	 * @param[in] type - send type
	 * @param[in] string - string to send
	 * @param[in] length - length of the string in bytes
	 * @return true on success, else false
	 * @remarks The actual option in performed in background. Call stop() to abort the operation.
	 * @remarks Ensure that the device outlives this operation.
	 * @remarks This command aborts any outstanding send operation.
	 */
	bool sendTo(pcf::serial::VkvmDevice & device, const SendType type, const char * string, const size_t length) {
		std::lock_guard<std::mutex> guard(this->sendToMutex);
		if (device.isConnected() == false || string == NULL || length <= 0) return false;
		if ( this->running ) {
			this->restarting = true;
			this->stop();
		} else if ( this->sendThread.joinable() ) {
			this->sendThread.join();
		}
		this->serialDevice = &device;
		this->str = static_cast<char *>(malloc(sizeof(char) * length));
		if (this->str == NULL) return false;
		memcpy(this->str, string, sizeof(char) * length);
		this->len = length;
		this->terminate = false;
		switch (type) {
		case SEND_ALT_CODE:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendAltCode, this);
			break;
		case SEND_ALT_X:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendAltXCode, this);
			break;
		case SEND_HEX_NUMPAD:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendHexNumpadCode, this);
			break;
		case SEND_ISO14755_HOLDING:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendIso14755CodeHolding, this);
			break;
		case SEND_ISO14755_HOLD_RELEASE:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendIso14755CodeHoldRelease, this);
			break;
		case SEND_VI:
			this->sendThread = std::thread(&VkvmControlSerialSend::sendViCode, this);
			break;
		}
		return true;
	}

	/**
	 * Returns the current background operation progress state.
	 *
	 * @return true if a background operation is in progress, else false
	 */
	inline bool inProgress() const {
		return this->running || this->restarting;
	}

	/**
	 * Sets the callback function called at operation completion.
	 *
	 * @param[in] fn - callback function pointer
	 * @param[in,out] widget - associated callback widget
	 * @param[in,out] arg - associated callback user argument
	 */
	inline void callback(Fl_Callback * fn, Fl_Widget * widget = NULL, void * arg = NULL) {
		this->cbFn = fn;
		this->cbWidget = widget;
		this->cbArg = arg;
	}

	/**
	 * Calls the associated callback function.
	 */
	inline void doCallback() const {
		if (this->cbFn == NULL) return;
		(*(this->cbFn))(this->cbWidget, this->cbArg);
	}

	/**
	 * Stops the current background operation.
	 *
	 * @return true if a background operation was in progress, else false
	 */
	inline bool stop() {
		if ( ! this->sendThread.joinable() ) return false;
		std::unique_lock<std::mutex> guard(this->terminateMutex);
		this->terminate = true;
		guard.unlock();
		this->terminateSignal.notify_one();
		this->sendThread.join();
		if (this->str != NULL) free(this->str);
		return true;
	}
private:
	/**
	 * Returns the USB key for the key which represents the hex value
	 * of the given nipple.
	 *
	 * @param[in] val - value to encode (nipple)
	 * @return USB key with the hex representation
	 */
	static inline uint8_t getHexKeyPad(const unsigned val) {
		static const uint8_t hexKeyPad[16] = {
			USBKEY_0, USBKEY_1, USBKEY_2, USBKEY_3,
			USBKEY_4, USBKEY_5, USBKEY_6, USBKEY_7,
			USBKEY_8, USBKEY_9, USBKEY_A, USBKEY_B,
			USBKEY_C, USBKEY_D, USBKEY_E, USBKEY_F
		};
		return hexKeyPad[val & 0xF];
	}

	/**
	 * Converts the current keyboard LED bits to modifier bits.
	 *
	 * @return Modifier bits from the LED bits.
	 */
	inline uint8_t ledsToMod() const {
		const uint8_t leds = this->serialDevice->keyboardLeds();
		uint8_t res = 0;
		if ((leds & USBLED_NUM_LOCK) != 0) res |= USBWRITE_RIGHT_NUM_LOCK;
		if ((leds & USBLED_KANA) != 0) res |= USBWRITE_RIGHT_KANA;
		return res;
	}

	/**
	 * Sends the given key codes. Retries on failure.
	 *
	 * @param[in] mod - modifier
	 * @param[in] modMask - mask of valid modifier bits
	 * @param[in] code - keyboard code array
	 * @param[in] codeLen - keyboard code array length
	 * @param[in] delayMs - delay in milliseconds between send attempts
	 * @return true on success, else false
	 */
	inline bool sendCode(uint8_t mod, const uint8_t modMask, const uint8_t * code, const uint8_t codeLen, const std::chrono::milliseconds::rep delayMs) const {
		const auto checkTerminate = [this]() {
			return this->terminate;
		};
		bool res;
		mod = uint8_t((mod & modMask) | (this->ledsToMod() & (~modMask)));
		do {
			std::unique_lock<std::mutex> guard(this->terminateMutex);
			/* send code */
			res = this->serialDevice->keyboardWrite(mod, code, codeLen);
			/* give the periphery time to process the key strokes and process stop requests */
			if ( this->terminateSignal.wait_for(guard, std::chrono::milliseconds(delayMs), checkTerminate) ) return false;
			/* terminate if the serial device connection got lost */
			if ( ! this->serialDevice->isConnected() ) return false;
		} while ( ! res ); /* retry until keyboardWrite succeeds */
		return true;
	}

	/**
	 * Sends the stored data as ALT code to the serial VKVM device.
	 */
	void sendAltCode() const {
		static const uint8_t keyPad[10] = {
			USBKEY_KP_0, USBKEY_KP_1, USBKEY_KP_2, USBKEY_KP_3, USBKEY_KP_4,
			USBKEY_KP_5, USBKEY_KP_6, USBKEY_KP_7, USBKEY_KP_8, USBKEY_KP_9
		};
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via ALT-Code */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[10];
			uint8_t codeRev[9];
			uint8_t codeLen;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint >= 128 && codePoint < 160) continue; /* no CP1252 equivalent */
				/* convert decimal Unicode code point to keypad number code */
				unsigned n = 0;
				for (; n < sizeof(codeRev) && codePoint != 0; n++, codePoint /= 10) {
					codeRev[n] = keyPad[codePoint % 10];
				}
				if (n >= sizeof(codeRev)) n = unsigned(sizeof(codeRev) - 1);
				/* reverse keypad number code */
				codeLen = 0;
				code[codeLen++] = keyPad[0]; /* leading zero */
				for (unsigned rN = 0; rN < n; rN++) {
					code[codeLen++] = codeRev[n - 1 - rN];
				}
				/* send CP1252 encoded ALT code */
				if ( ! this->sendCode(USBWRITE_RIGHT_NUM_LOCK | USBWRITE_LEFT_ALT, ALL_KEYS | NUMLOCK_LED, code, codeLen, 20) ) {
					return;
				}
			}
		} catch (...) {}
	}

	/**
	 * Sends the stored data as ALT-X code to the serial VKVM device.
	 */
	void sendAltXCode() const {
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via ISO/IEC 14755 code */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[8];
			code[6] = USBKEY_LEFT_ALT;
			code[7] = USBKEY_X;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint > 0xFFFFFF) continue; /* out of 6 hex character range */
				/* convert decimal Unicode code point to hexadecimal Unicode number */
				code[0] = getHexKeyPad((codePoint >> 20) & 0x0F);
				code[1] = getHexKeyPad((codePoint >> 16) & 0x0F);
				code[2] = getHexKeyPad((codePoint >> 12) & 0x0F);
				code[3] = getHexKeyPad((codePoint >> 8) & 0x0F);
				code[4] = getHexKeyPad((codePoint >> 4) & 0x0F);
				code[5] = getHexKeyPad(codePoint & 0x0F);
				/* send ALT-X encoded code point and ALT-X */
				if ( ! this->sendCode(USBWRITE_NONE, USBWRITE_NONE, code, sizeof(code), 100) ) {
					return;
				}
			}
		} catch (...) {}
	}

	/**
	 * Sends the stored data as hex numpad code to the serial VKVM device.
	 */
	void sendHexNumpadCode() const {
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via hex numpad code */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[5];
			code[0] = USBKEY_KP_ADD;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint > 0xFFFF) continue; /* out of UCS range */
				/* convert decimal Unicode code point to hexadecimal UCS number */
				code[1] = getHexKeyPad((codePoint >> 12) & 0x0F);
				code[2] = getHexKeyPad((codePoint >> 8) & 0x0F);
				code[3] = getHexKeyPad((codePoint >> 4) & 0x0F);
				code[4] = getHexKeyPad(codePoint & 0x0F);
				/* send hex numpad encoded code point */
				if ( ! this->sendCode(USBWRITE_LEFT_ALT, ALL_KEYS, code, sizeof(code), 100) ) {
					return;
				}
			}
		} catch (...) {}
	}

	/**
	 * Sends the stored data as ISO/IEC 14755 code to the serial VKVM device holding CTRL-SHIFT.
	 */
	void sendIso14755CodeHolding() const {
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via ISO/IEC 14755 code */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[5];
			code[0] = USBKEY_U;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint > 0xFFFF) continue; /* out of UCS range */
				/* convert decimal Unicode code point to hexadecimal UCS number code */
				code[1] = getHexKeyPad((codePoint >> 12) & 0x0F);
				code[2] = getHexKeyPad((codePoint >> 8) & 0x0F);
				code[3] = getHexKeyPad((codePoint >> 4) & 0x0F);
				code[4] = getHexKeyPad(codePoint & 0x0F);
				/* send ISO/IEC 14755 encoded code point */
				if ( ! this->sendCode(USBWRITE_LEFT_SHIFT | USBWRITE_LEFT_CONTROL, ALL_KEYS, code, sizeof(code), 120) ) {
					return;
				}
			}
		} catch (...) {}
	}

	/**
	 * Sends the stored data as ISO/IEC 14755 code to the serial VKVM device hold/release CTRL-SHIFT.
	 */
	void sendIso14755CodeHoldRelease() const {
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via ISO/IEC 14755 code */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[8];
			code[0] = USBKEY_U;
			code[1] = USBKEY_LEFT_CONTROL;
			code[2] = USBKEY_LEFT_SHIFT;
			code[7] = USBKEY_ENTER;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint > 0xFFFF) continue; /* out of UCS range */
				/* convert decimal Unicode code point to hexadecimal UCS number code */
				code[3] = getHexKeyPad((codePoint >> 12) & 0x0F);
				code[4] = getHexKeyPad((codePoint >> 8) & 0x0F);
				code[5] = getHexKeyPad((codePoint >> 4) & 0x0F);
				code[6] = getHexKeyPad(codePoint & 0x0F);
				/* send ISO/IEC 14755 encoded code point */
				if ( ! this->sendCode(USBWRITE_LEFT_SHIFT | USBWRITE_LEFT_CONTROL, ALL_KEYS, code, sizeof(code), 120) ) {
					return;
				}
			}
		} catch (...) {}
	}

	/**
	 * Sends the stored data as Vi/Vim code to the serial VKVM device.
	 */
	void sendViCode() const {
		this->running = true;
		this->restarting = false; /* set after running to ensure proper response in inProgress() */
		try {
			const auto doCallbackOnReturn = makeScopeExit([this]() {
				this->running = false;
				this->doCallback();
			});
			if (this->str == NULL || this->len <= 0 || this->serialDevice == NULL) return;
			if ( ! this->serialDevice->isConnected() ) return;
			/* paste via Vi/Vim code (e.g. U+0020 as CTRL-V, u, 0, 0, 2, 0) */
			int bytes = 0;
			const char * end = this->str + this->len;
			uint8_t code[7];
			code[0] = USBKEY_V;
			code[1] = USBKEY_LEFT_CONTROL; /* revert modifier */
			code[2] = USBKEY_U;
			for (const char * ptr = this->str; ptr < end; ptr += bytes) {
				unsigned codePoint = fl_utf8decode(ptr, end, &bytes);
				if (codePoint == 0) break;
				if (codePoint > 0xFFFF) continue; /* out of UCS range */
				/* convert decimal Unicode code point to hexadecimal UCS number code */
				code[3] = getHexKeyPad((codePoint >> 12) & 0x0F);
				code[4] = getHexKeyPad((codePoint >> 8) & 0x0F);
				code[5] = getHexKeyPad((codePoint >> 4) & 0x0F);
				code[6] = getHexKeyPad(codePoint & 0x0F);
				/* send Vi/Vim encoded code point */
				if ( ! this->sendCode(USBWRITE_LEFT_CONTROL, ALL_KEYS, code, sizeof(code), 20) ) {
					return;
				}
			}
		} catch (...) {}
	}
};


/**
 * Rotation choice popup.
 */
class VkvmControlRotationPopup : public Fl_Double_Window {
private:
	SvgButton * item[4];
	bool revert;
	bool done;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] W - popup window width
	 * @param[in] H - popup window height
	 */
	explicit inline VkvmControlRotationPopup(const int W, const int H):
		Fl_Double_Window(W + Fl::box_dw(FL_THIN_UP_BOX), H + Fl::box_dh(FL_THIN_UP_BOX)),
		revert(false),
		done(false)
	{
		box(FL_THIN_UP_BOX);
		const int itemWidth = W / 4;
		int x1 = Fl::box_dx(FL_THIN_UP_BOX);
		const int y1 = Fl::box_dy(FL_THIN_UP_BOX);

		for (int n = 0; n < 4; n++) {
			item[n] = new SvgButton(x1, y1, itemWidth, H, getRotationSvg(VkvmView::Rotation(n)));
			item[n]->type(FL_RADIO_BUTTON);
			item[n]->hover(true);
			item[n]->colorButton(true);
			item[n]->selection_color(FL_FOREGROUND_COLOR);
			item[n]->labelcolor(FL_FOREGROUND_COLOR);
			x1 += itemWidth;
		}

		end();

		clear_border();
		set_modal();
		set_menu_window();
	}

	/**
	 * Destructor.
	 */
	virtual ~VkvmControlRotationPopup() {}

	/**
	 * Shows the rotation popup window at the given position and with the
	 * passed current rotation value.
	 *
	 * @param[in] X - absolute screen position X
	 * @param[in] Y - absolute screen position Y
	 * @param[in] ROT_DEFAULT - current rotation
	 * @return rotation selected by the user
	 */
	inline VkvmView::Rotation show(const int X, const int Y, const VkvmView::Rotation rot = VkvmView::ROT_DEFAULT) {
		this->item[int(rot)]->setonly();
		this->position(X, Y);
		/* grab this window (capture target, however, is the current topmost window) */
		Fl::grab(*this);
		/* the window is created from here on */
		this->Fl_Double_Window::show();
		this->revert = false;
		this->done = false;
		while ( ! this->done ) Fl::wait();
		Fl::grab(NULL);
		if ( ! this->revert ) {
			return VkvmView::Rotation(this->getSelectedItem());
		}
		return rot;
	}
protected:
	/**
	 * Event handler.
	 *
	 * @param[in] e - event
	 * @return 0 - if the event was not used or understood
	 * @return 1 - if the event was used and can be deleted
	 */
	int handle(int e) {
		int result = Fl_Double_Window::handle(e);
		switch (e) {
		case FL_UNFOCUS:
			if ( ! contains(Fl::focus()) ) {
				revert = true;
				hide();
				result = 1;
			}
			break;
		case FL_PUSH:
			for (int n = 0; n < 4; n++) {
				if (Fl::event_inside(item[n]) != 0) {
					item[n]->setonly();
					break;
				}
			}
			hide();
			result = 1;
			break;
		case FL_KEYBOARD:
			switch (Fl::event_key()) {
			case FL_Escape:
				revert = true;
				/* fall-through */
			case FL_KP_Enter:
			case FL_Enter:
				hide();
				result = 1;
				break;
			case FL_Left:
				item[(getSelectedItem() + 3) % 4]->setonly();
				result = 1;
				break;
			case FL_Right:
				item[(getSelectedItem() + 1) % 4]->setonly();
				result = 1;
				break;
			default:
				break;
			}
			break;
		case FL_HIDE:
			done = true;
			result = 1;
			break;
		default:
			break;
		}
		return result;
	}
private:
	/* default show() method is not allowed */
	using Fl_Double_Window::show;

	/**
	 * Returns the index to the selected item.
	 *
	 * @return selected item index or 0 if none is selected (default)
	 */
	inline int getSelectedItem() const {
		for (int n = 1; n < 4; n++) {
			if (this->item[n]->value() != 0) return n;
		}
		return 0;
	}
};


/**
 * Status history popup.
 */
class VkvmControlStatusPopup : public Fl_Double_Window {
private:
	enum {
		MAX_LINE = 256,
		MAX_HISTORY = 10
	};
	StatusItem history[MAX_HISTORY];
	Fl_Label item;
	int minWidth;
	int itemHeight;
	bool done;
public:
	/**
	 * Constructor.
	 *
	 * @param[in] W - popup window width
	 * @param[in] H - popup window height
	 */
	explicit inline VkvmControlStatusPopup(const int W, const int H):
		Fl_Double_Window(W, H),
		minWidth(W),
		itemHeight(H),
		done(false)
	{
		box(FL_THIN_DOWN_BOX);
		item.value = NULL;
		item.image = NULL;
		item.deimage = NULL;
		item.type = FL_NORMAL_LABEL;
		item.font = FL_HELVETICA;
		item.size = FL_NORMAL_SIZE;
		item.color = FL_FOREGROUND_COLOR;
		item.align_ = FL_ALIGN_CENTER;
		end();

		clear_border();
		set_modal();
		set_menu_window();
	}

	/**
	 * Destructor.
	 */
	virtual ~VkvmControlStatusPopup() {}

	/**
	 * Adds an additional status line to the history.
	 *
	 * @param[in] text - text to add
	 * @param[in] copy - true to copy the text, false to use it
	 */
	inline void addStatusLine(const char * text, const bool copy) {
		if (text == NULL || *text == 0) return; /* ignore empty lines */
		/* move old items to make space for the new item */
		for (size_t n = 1; n < MAX_HISTORY; n++) {
			this->history[MAX_HISTORY - n] = std::move(this->history[MAX_HISTORY - n - 1]);
		}
		this->history[0] = StatusItem(time(NULL), text, copy);
	}

	/**
	 * Shows the status history popup window at the given position.
	 *
	 * @param[in] X - absolute screen position X
	 * @param[in] Y - absolute screen position Y
	 * @param[in] W - minimal popup window width
	 */
	void show(const int X, const int Y, const int W = 0) {
		/* update size according to items in history */
		const Fl_Boxtype b = box();
		int minW = 0, minH = 0;
		int lw, lh;
		char buf[MAX_LINE];
		if (W != 0) this->minWidth = W;
		for (size_t n = 0; n < MAX_HISTORY; n++) {
			if ( ! this->history[n] ) break;
			const struct tm * dt = localtime(&(this->history[n].dateTime));
			strftime(buf, sizeof(buf), "[%H:%M:%S] ", dt);
			snprintf(buf + 11, sizeof(buf) - 11, "%s", this->history[n].text);
			item.value = buf;
			lw = 0;
			lh = 0;
			item.measure(lw, lh);
			if (lw > minW) minW = lw;
			minH += this->itemHeight;
		}
		if (minW == 0 || minH == 0) return; /* nothing to show */
		minW += Fl::box_dw(b) + 6;
		minH += Fl::box_dh(b);
		if (this->minWidth > minW) minW = minWidth;
		this->resize(X, Y - minH, minW, minH);
		/* grab this window (capture target, however, is the current topmost window) */
		Fl::grab(*this);
		/* the window is created from here on */
		this->Fl_Double_Window::show();
		this->done = false;
		while ( ! this->done ) Fl::wait();
		Fl::grab(NULL);
	}
protected:
	/**
	 * Event handler.
	 *
	 * @param[in] e - event
	 * @return 0 - if the event was not used or understood
	 * @return 1 - if the event was used and can be deleted
	 */
	int handle(int e) {
		int result = Fl_Double_Window::handle(e);
		switch (e) {
		case FL_UNFOCUS:
			if ( ! contains(Fl::focus()) ) {
				hide();
				result = 1;
			}
			break;
		case FL_PUSH:
			hide();
			result = 1;
			break;
		case FL_KEYBOARD:
			switch (Fl::event_key()) {
			case FL_KP_Enter:
			case FL_Enter:
			case FL_Escape:
				hide();
				result = 1;
				break;
			default:
				break;
			}
			break;
		case FL_HIDE:
			done = true;
			result = 1;
			break;
		default:
			break;
		}
		return result;
	}

	/**
	 * Draws the history items. This should never be called directly. Use redraw() instead.
	 */
	void draw() {
		if (w() <= 0 || h() <= 0 || !visible()) return;
		const Fl_Boxtype b = box();
		const int dx = Fl::box_dx(b) + 3;
		const int dy = Fl::box_dy(b);
		const int itemWidth = w() - Fl::box_dw(b) - 6;
		draw_box(b, 0, 0, w(), h(), color());
		int y1 = h() - itemHeight - dy;
		char buf[MAX_LINE];
		for (size_t n = 0; n < MAX_HISTORY; n++) {
			if ( ! this->history[n] ) break;
			const struct tm * dt = localtime(&(this->history[n].dateTime));
			strftime(buf, sizeof(buf), "[%H:%M:%S] ", dt);
			snprintf(buf + 11, sizeof(buf) - 11, "%s", this->history[n].text);
			item.value = buf;
			item.draw(dx, y1, itemWidth, itemHeight, FL_ALIGN_LEFT);
			y1 -= itemHeight;
		}
	}
private:
	/* default show() method is not allowed */
	using Fl_Double_Window::show;

};


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - window title
 */
VkvmControl::VkvmControl(const int X, const int Y, const int W, const int H, const char * L):
	Fl_Double_Window(X, Y, W, H, (L != NULL) ? L : "VKVM")
{
	this->init();
}


/**
 * Constructor.
 *
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - window title
 */
VkvmControl::VkvmControl(const int W, const int H, const char * L):
	Fl_Double_Window(W, H, (L != NULL) ? L : "VKVM")
{
	this->init();
}


/**
 * Constructor.
 */
void VkvmControl::init() {
	const int W = this->w();
	const int H = this->h();
	const int sizeH = adjDpiH(26);
	const int sizeV = adjDpiV(26);
	const int dx = 1;
	int y1 = 0;
	int x1 = 1;

	serialSend = NULL;
	serialOn = false;
	serialChange = false;
	toolbar = NULL;
	sourceList = NULL;
	videoConfig = NULL;
	aspectRatio = NULL;
	mirrorRight = NULL;
	mirrorUp = NULL;
	rotation = NULL;
	fullscreen = NULL;
	serialList = NULL;
	videoFrame = NULL;
	video = NULL;
	status1 = NULL;
	statusConnection = NULL;
	statusNumLock = NULL;
	statusCapsLock = NULL;
	statusScrollLock = NULL;
	rotationPopup = NULL;
	statusHistory = NULL;
	addedWidth = 0;
	addedHeight = 0;
	minWidth = 0;
	redirectInput = false;
	Fl::get_mouse(lastMouseX, lastMouseY);
	lastReason = DisconnectReason::COUNT;
	licenseWin = NULL;

	/* changes here need to be done also in VkvmControl::onSendKey() and VkvmControl::onPaste() */
	static const Fl_Menu_Item sendKeyDropDownMenu[] = {
		{"Win: paste via ALT code", 0, NULL, NULL, FL_MENU_RADIO},
		{"Win: paste via ALT-X code", 0, NULL, NULL, FL_MENU_RADIO},
		{"Win: paste via hex numpad", 0, NULL, NULL, FL_MENU_RADIO},
		{"X11: paste by holding CTRL-SHIFT-U (ISO/IEC 14755)", 0, NULL, NULL, FL_MENU_RADIO},
		/* intended trailing space: */
		{"X11: paste with hold/release CTRL-SHIFT-U (ISO/IEC 14755) ", 0, NULL, NULL, FL_MENU_RADIO},
		{"paste via Vi/Vim code", 0, NULL, NULL, FL_MENU_RADIO},
		{"ALT-F4", 0, NULL, NULL, FL_MENU_RADIO},
		{"CTRL-ALT-DEL", 0, NULL, NULL, FL_MENU_RADIO},
		{NULL}
	};

	toolbar = new Fl_Group(0, y1, W, sizeV);
	toolbar->box(FL_THIN_UP_BOX);
	{
		const int y2 = y1 + toolbar->x() + Fl::box_dy(toolbar->box());
		/* video sources */
		sourceList = new HoverChoice(x1, y2, adjDpiH(160), sizeV - (2 * dx));
		x1 += sourceList->w();
		/* video config */
		videoConfig = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), settingsSvg);
		videoConfig->callback(PCF_GUI_CALLBACK(onVideoConfig), this);
		videoConfig->colorButton(true);
		videoConfig->selection_color(FL_FOREGROUND_COLOR);
		videoConfig->hover(true);
		videoConfig->tooltip("video configuration");
		x1 += videoConfig->w();
		/* aspect ratio */
		aspectRatio = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), aspectRatioSvg);
		aspectRatio->callback(PCF_GUI_CALLBACK(onFixWindowSize), this);
		aspectRatio->colorButton(true);
		aspectRatio->selection_color(FL_FOREGROUND_COLOR);
		aspectRatio->hover(true);
		aspectRatio->tooltip("resize to match aspect ratio");
		x1 += aspectRatio->w();
		/* mirror/flip right */
		mirrorRight = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), mirrorRightSvg);
		mirrorRight->type(FL_TOGGLE_BUTTON);
		mirrorRight->callback(PCF_GUI_CALLBACK(onMirrorRight), this);
		mirrorRight->colorButton(true);
		mirrorRight->selection_color(FL_FOREGROUND_COLOR);
		mirrorRight->hover(true);
		mirrorRight->tooltip("mirror horizontal");
		x1 += mirrorRight->w();
		/* mirror/flip up */
		mirrorUp = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), mirrorUpSvg);
		mirrorUp->type(FL_TOGGLE_BUTTON);
		mirrorUp->callback(PCF_GUI_CALLBACK(onMirrorUp), this);
		mirrorUp->colorButton(true);
		mirrorUp->selection_color(FL_FOREGROUND_COLOR);
		mirrorUp->hover(true);
		mirrorUp->tooltip("mirror vertical");
		x1 += mirrorUp->w();
		/* rotate */
		rotation = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), upSvg);
		rotation->callback(PCF_GUI_CALLBACK(onRotation), this);
		rotation->colorButton(true);
		rotation->selection_color(FL_FOREGROUND_COLOR);
		rotation->hover(true);
		rotation->tooltip("rotation");
		x1 += rotation->w();
		/* fullscreen */
		fullscreen = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), fullscreenSvg);
		fullscreen->type(FL_TOGGLE_BUTTON);
		fullscreen->callback(PCF_GUI_CALLBACK(onFullscreen), this);
		fullscreen->colorButton(true);
		fullscreen->selection_color(FL_FOREGROUND_COLOR);
		fullscreen->hover(true);
		fullscreen->tooltip("fullscreen mode");
		x1 += fullscreen->w();
		/* serial ports */
		serialList = new HoverChoice(x1, y2, adjDpiH(160), sizeV - (2 * dx));
		x1 += serialList->w();
		/* send key */
		sendKey = new SvgButton(x1, y2, sizeH - (2 * dx), sizeV - (2 * dx), sendKeySvg);
		sendKey->callback(PCF_GUI_CALLBACK(onSendKey), this);
		sendKey->colorButton(true);
		sendKey->labelcolor(SERIAL_COLOR_PASTE_COMPLETE);
		sendKey->hover(true);
		sendKey->tooltip("send key(s)");
		x1 += sendKey->w();
		/* send key drop-down */
		sendKeyChoice = new SvgButton(x1, y2, (((sizeH - (2 * dx)) * 4) + 5) / 10, sizeV - (2 * dx), dropDownSvg);
		sendKeyChoice->callback(PCF_GUI_CALLBACK(onSendKeyChoice), this);
		sendKeyChoice->colorButton(true);
		sendKeyChoice->selection_color(FL_FOREGROUND_COLOR);
		sendKeyChoice->hover(true);
		sendKeyChoice->linkHoverState(sendKey);
		sendKeyDropDown = new HoverDropDown();
		sendKeyDropDown->copy(sendKeyDropDownMenu);
		sendKeyDropDown->setonly(const_cast<Fl_Menu_Item *>(sendKeyDropDown->menu())); /* select first item */
		x1 += sendKeyChoice->w();
		/* filler */
		Fl_Box * filler = new Fl_Box(x1, y2, W - x1 - sizeH + dx, sizeV - (2 * dx));
		filler->hide();
		/* license */
		license = new SvgButton(filler->x() + filler->w(), y2, sizeH - (2 * dx), sizeV - (2 * dx), licenseSvg);
		license->callback(PCF_GUI_CALLBACK(onLicense), this);
		license->colorButton(true);
		license->selection_color(FL_FOREGROUND_COLOR);
		license->hover(true);
		x1 += license->w();
		toolbar->resizable(filler);
	}
	y1 += (sizeV + 2); /* tool bar frame */
	toolbar->end();

	/* video */
	videoFrame = new Fl_Group(0, y1, W, H - sizeV - y1);
	videoFrame->box(FL_NO_BOX);
	{
		video = new VkvmView(0, y1, W, H - sizeV - y1);
		video->captureResizeCallback(PCF_GUI_CALLBACK(onVideoResize), this);
		video->clickCallback(PCF_GUI_CALLBACK(onVideoClick), this);
	}
	videoFrame->end();

	/* status bar */
	{
		Fl_Group * status = new Fl_Group(0, H - sizeV + 1, W, sizeV - 1);
		int x2 = 0;

		/* general status field */
		status1 = new OnClickBox(0, H - sizeV + 1, W - (4 * sizeH) - dx, sizeV - 1);
		status1->box(FL_THIN_DOWN_BOX);
		status1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
		status1->labeltype(FL_NO_SYMBOL_LABEL);
		status1->callback(PCF_GUI_CALLBACK(onStatusClick), this);
		x2 += status1->w();
		{
			Fl_Group * status2 = new Fl_Group(x2, H - sizeV + 1, 4 * sizeV, sizeV - 1);
			status2->box(FL_THIN_DOWN_BOX);
			/* serial connection indicator */
			statusConnection = new SvgView(x2 + dx, H - sizeV + dx + 1, sizeH - (2 * dx), sizeV - (2 * dx) - 1, disconnectedSvg);
			statusConnection->box(FL_NO_BOX);
			statusConnection->colorView(true);
			statusConnection->selection_color(SERIAL_COLOR_DISCONNECTED);
			statusConnection->tooltip("serial connection");
			x2 += sizeH;
			/* num lock indicator */
			statusNumLock = new SvgView(x2 + dx, H - sizeV + dx + 1, sizeH - (2 * dx), sizeV - (2 * dx) - 1, numLockSvg);
			statusNumLock->box(FL_NO_BOX);
			statusNumLock->colorView(true);
			statusNumLock->selection_color(STATUS_COLOR_LED_OFF);
			statusNumLock->tooltip("num lock");
			x2 += sizeH;
			/* caps lock indicator */
			statusCapsLock = new SvgView(x2 + dx, H - sizeV + dx + 1, sizeH - (2 * dx), sizeV - (2 * dx) - 1, capsLockSvg);
			statusCapsLock->box(FL_NO_BOX);
			statusCapsLock->colorView(true);
			statusCapsLock->selection_color(STATUS_COLOR_LED_OFF);
			statusCapsLock->tooltip("caps lock");
			x2 += sizeH;
			/* scroll lock indicator */
			statusScrollLock = new SvgView(x2 + dx, H - sizeV + dx + 1, sizeH - (2 * dx), sizeV - (2 * dx) - 1, scrollLockSvg);
			statusScrollLock->box(FL_NO_BOX);
			statusScrollLock->colorView(true);
			statusScrollLock->selection_color(STATUS_COLOR_LED_OFF);
			statusScrollLock->tooltip("scroll lock");
			status2->end();
		}

		status->end();
		status->resizable(status1);
		y1 += sizeV;
	}
	end();

	addedWidth = w() - videoFrame->w();
	addedHeight = h() - videoFrame->h();

	callback(PCF_GUI_CALLBACK(onQuit), this);
	resizable(videoFrame);
	if (w() < (x1 + dx) || h() < (y1 + dx)) size(x1 + dx, y1 + dx);
	size_range(x1 + dx, y1 + dx);
	minWidth = x1 + dx;

	rotationPopup = new VkvmControlRotationPopup(rotation->w() * 4, rotation->h());
	statusHistory = new VkvmControlStatusPopup(status1->w(), status1->h() - Fl::box_dh(status1->box()));

	onCaptureDeviceChange();
	onSerialPortChange();
	onSerialConnectionChange();
	videoSource.addNotificationCallback(*this);
	serialPortSource.addNotificationCallback(*this);
	licenseWin = new LicenseInfoWindow(adjDpiH(600), adjDpiV(600), "About VKVM " VKVM_VERSION);
	serialSend = new VkvmControlSerialSend;
	serialSend->callback(PCF_GUI_CALLBACK(onPasteComplete), sendKey, this);
}


VkvmControl::~VkvmControl() {
	if (serialSend != NULL) delete serialSend; /* abort outstanding operations early */
	stopInputCapture();
	videoSource.removeNotificationCallback(*this);
	videoSource.freeDeviceList(videoDevices);
	/* containing widgets are deleted by the base class */
	if (licenseWin != NULL) delete licenseWin;
	if (rotationPopup != NULL) delete rotationPopup;
	if (statusHistory != NULL) delete statusHistory;
}


int VkvmControl::handle(int e) {
	if ( this->redirectInput ) {
		switch (e) {
		case FL_PASTE:
			this->onPaste(Fl::event_text(), Fl::event_length());
			break;
		case FL_UNFOCUS:
		case FL_CLOSE:
		case FL_HIDE:
			this->stopInputCapture();
			break;
		default:
			break;
		}
		return 1;
	}
	int res = 0;
	switch (e) {
	case FL_PASTE:
		this->onPaste(Fl::event_text(), Fl::event_length());
		res = 1;
		break;
	case FL_SHORTCUT:
		if (Fl::event_key(FL_META) != 0) break;
		if (Fl::event_ctrl() != 0 && Fl::event_alt() == 0 && Fl::event_shift() == 0) {
			/* only CTRL is held down */
			switch (Fl::event_key()) {
			case FL_Up:
				this->setRotation(VkvmView::ROT_UP);
				res = 1;
				break;
			case FL_Right:
				this->setRotation(VkvmView::ROT_RIGHT);
				res = 1;
				break;
			case FL_Down:
				this->setRotation(VkvmView::ROT_DOWN);
				res = 1;
				break;
			case FL_Left:
				this->setRotation(VkvmView::ROT_LEFT);
				res = 1;
				break;
			case 'f':
				/* toggle fullscreen mode */
				if (this->fullscreen != NULL) {
					this->fullscreen->value((this->fullscreen->value() != 0) ? 0 : 1);
					this->fullscreen->redraw();
					this->fullscreen->do_callback();
				}
				res = 1;
				break;
			case 'k':
				/* CTRL-K for screen less control */
				this->startInputCapture();
				res = 1;
				break;
			default:
				break;
			}
		} else if (Fl::event_ctrl() == 0 && Fl::event_alt() != 0 && Fl::event_shift() == 0) {
			/* only ALT is held down */
			switch (Fl::event_key()) {
			case FL_Right:
			case FL_Left:
				/* toggle mirror/flip right */
				if (this->mirrorRight != NULL) {
					this->mirrorRight->value((this->mirrorRight->value() != 0) ? 0 : 1);
					this->mirrorRight->redraw();
					this->mirrorRight->do_callback();
				}
				res = 1;
				break;
			case FL_Up:
			case FL_Down:
				/* toggle mirror/flip up */
				if (this->mirrorUp != NULL) {
					this->mirrorUp->value((this->mirrorUp->value() != 0) ? 0 : 1);
					this->mirrorUp->redraw();
					this->mirrorUp->do_callback();
				}
				res = 1;
				break;
			default:
				break;
			}
		}
		break;
	case FL_FULLSCREEN:
		this->fullscreen->value(this->fullscreen_active() ? 1 : 0);
		break;
	default:
		break;
	}
	return (res == 1) ? res : Fl_Double_Window::handle(e);
}


void VkvmControl::resize(int X, int Y, int W, int H) {
	this->Fl_Double_Window::resize(X, Y, W, H);
	/* keep video aspect ratio of the camera view */
	if (this->video == NULL || this->video->captureDevice() == NULL) return;
	const size_t captureWidth = this->video->captureWidth();
	const size_t captureHeight = this->video->captureHeight();
	if (captureWidth <= 1 || captureHeight <= 1) {
		this->video->size(this->videoFrame->w(), this->videoFrame->h());
		return;
	}
	const int videoX = this->videoFrame->x();
	const int videoY = this->videoFrame->y();
	const int videoWidth = this->videoFrame->w();
	const int videoHeight = this->videoFrame->h();
	const int newWidth = int((float(captureWidth) * float(videoHeight) / float(captureHeight)) + 0.5f);
	const int newHeight = int((float(captureHeight) * float(videoWidth) / float(captureWidth)) + 0.5f);
	/* only resize to smaller dimensions */
	if (newWidth < videoWidth) {
		this->video->resize(videoX + (videoWidth - newWidth) / 2, videoY, newWidth, videoHeight);
	} else {
		this->video->resize(videoX, videoY + (videoHeight - newHeight) / 2, videoWidth, newHeight);
	}
}


void VkvmControl::onVideoSource(Fl_Window * /* w */) {
	const int index = this->sourceList->value();
	if (index <= 0 || (index - 1) >= int(this->videoDevices.size())) {
		/* first item is a dummy -> remove capture device */
		this->video->captureDevice(NULL);
	} else {
		if ( ! this->video->captureDevice(this->videoDevices[size_t(index - 1)]) ) {
			this->setStatusLine("Failed to start video capture.");
			/* first item is a dummy -> remove capture device */
			this->sourceList->value(0);
			this->video->captureDevice(NULL);
		}
	}
	this->onCaptureViewChange();
}


void VkvmControl::onVideoConfig(SvgButton * /* tool */) {
	pcf::video::CaptureDevice * device = this->video->captureDevice();
	if (device != NULL) device->configure(fl_xid(this->top_window()));
}


void VkvmControl::onFixWindowSize(SvgButton * /* tool */) {
	if (!this->visible() || this->fullscreen_active()) return;
	if (this->video == NULL || this->video->captureDevice() == NULL) return;
	const size_t captureWidth = this->video->captureWidth();
	const size_t captureHeight = this->video->captureHeight();
	if (captureWidth <= 1 || captureHeight <= 1) return;
	const int videoWidth = this->videoFrame->w();
	const int videoHeight = this->videoFrame->h();
	/* adjust window size to keep video capture aspect ratio */
	const int newWidth = int((float(captureWidth) * float(videoHeight) / float(captureHeight)) + 0.5f);
	const int newHeight = int((float(captureHeight) * float(videoWidth) / float(captureWidth)) + 0.5f);
	/* only resize to smaller dimensions if larger than lower limits */
	if (newWidth < videoWidth && newWidth >= minWidth) {
		this->size(newWidth + this->addedWidth, this->h());
	} else {
		this->size(this->w(), newHeight + this->addedHeight);
	}
}


void VkvmControl::onRotation(SvgButton * tool) {
	if (tool == NULL || this->video == NULL || this->rotationPopup == NULL) return;
	this->setRotation(this->rotationPopup->show(this->x() + tool->x(), this->y() + tool->y() + tool->h(), this->video->rotation()));
}


void VkvmControl::onMirrorRight(SvgButton * tool) {
	if (tool == NULL || this->video == NULL) return;
	this->video->mirrorRight(tool->value() != 0);
}


void VkvmControl::onMirrorUp(SvgButton * tool) {
	if (tool == NULL || this->video == NULL) return;
	this->video->mirrorUp(tool->value() != 0);
}


void VkvmControl::onFullscreen(SvgButton * tool) {
	if (tool == NULL) return;
	if ( tool->value() ) {
		this->Fl_Double_Window::fullscreen();
	} else {
		this->Fl_Double_Window::fullscreen_off();
	}
}


void VkvmControl::onSerialSource(Fl_Window * /* w */) {
	const int index = this->serialList->value();
	if (index == 0 || (index - 1) >= int(this->serialPorts.size())) {
		/* first item is a dummy -> disconnect serial port */
		if ( this->serialOn ) this->disconnectPeriphery();
		this->serialPort = pcf::serial::SerialPort();
		if (this->statusConnection != NULL) this->statusConnection->deactivate();
	} else {
		this->serialPort = this->serialPorts[size_t(index - 1)];
		if ( this->serialOn ) {
			this->serialChange = true;
			this->disconnectPeriphery();
		} else {
			this->serialOn = true;
			this->serialChange = false;
			this->connectPeriphery();
		}
		if (this->statusConnection != NULL) this->statusConnection->activate();
	}
}


void VkvmControl::onSendKey(SvgButton * /* tool */) {
	if (this->sendKeyDropDown == NULL) return;
	const Fl_Menu_Item * m = this->sendKeyDropDown->menu();
	if (m == NULL) return;
	if (Fl::event_button() == FL_RIGHT_MOUSE) {
		this->onSendKeyChoice(this->sendKeyChoice);
		return;
	}
	if (m[0].value() != 0) {
		/* paste via ALT code */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[1].value() != 0) {
		/* paste via ALT-X code */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[2].value() != 0) {
		/* paste via hex numpad code */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[3].value() != 0) {
		/* paste via ISO/IEC 14755 code holding CTRL-SHIFT */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[4].value() != 0) {
		/* paste via ISO/IEC 14755 code hold/release CTRL-SHIFT */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[5].value() != 0) {
		/* paste via Vi/Vim code */
		if (Fl::clipboard_contains(Fl::clipboard_plain_text) == 0) return;
		Fl::paste(*(this->as_window()), 1); /* see VkvmControl::onPaste() */
	} else if (m[6].value() != 0) {
		/* ALT-F4 */
		if ( ! this->serialDevice.isConnected() ) return;
		this->serialSend->stop();
		this->serialDevice.keyboardDown(USBKEY_LEFT_ALT);
		this->serialDevice.keyboardPush(USBKEY_F4);
		this->serialDevice.keyboardUp(USBKEY_LEFT_ALT);
	} else if (m[7].value() != 0) {
		/* CTRL-ALT-DEL */
		if ( ! this->serialDevice.isConnected() ) return;
		this->serialSend->stop();
		this->serialDevice.keyboardDown(USBKEY_LEFT_CONTROL);
		this->serialDevice.keyboardDown(USBKEY_LEFT_ALT);
		this->serialDevice.keyboardPush(USBKEY_DELETE);
		this->serialDevice.keyboardUp(USBKEY_LEFT_ALT);
		this->serialDevice.keyboardUp(USBKEY_LEFT_CONTROL);
	}
}


void VkvmControl::onSendKeyChoice(SvgButton * /* tool */) {
	if (this->sendKey == NULL || this->sendKeyDropDown == NULL) return;
	this->sendKeyDropDown->dropDown(this->sendKey->x(), this->sendKey->y() + this->sendKey->h());
}


void VkvmControl::onPaste(const char * str, const int len) {
	if (str == NULL || len <= 0 || this->sendKey == NULL || this->sendKeyDropDown == NULL || this->statusConnection == NULL || this->serialSend == NULL) return;
	if ( ! this->serialDevice.isConnected() ) return;
	const Fl_Menu_Item * m = this->sendKeyDropDown->menu();
	if (m == NULL) return;
	bool pasted = false;
	if (m[0].value() != 0) {
		/* paste via ALT code */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_ALT_CODE, str, size_t(len));
	} else if (m[1].value() != 0) {
		/* paste via ALT-X code */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_ALT_X, str, size_t(len));
	} else if (m[2].value() != 0) {
		/* paste via hex numpad code */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_HEX_NUMPAD, str, size_t(len));
	} else if (m[3].value() != 0) {
		/* paste via ISO/IEC 14755 holding CTRL-SHIFT */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_ISO14755_HOLDING, str, size_t(len));
	} else if (m[4].value() != 0) {
		/* paste via ISO/IEC 14755 hold/release CTRL-SHIFT */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_ISO14755_HOLD_RELEASE, str, size_t(len));
	} else if (m[5].value() != 0) {
		/* paste via Vi/Vim */
		pasted = this->serialSend->sendTo(this->serialDevice, VkvmControlSerialSend::SEND_VI, str, size_t(len));
	}
	if ( pasted ) {
		this->sendKey->labelcolor(SERIAL_COLOR_PASTE_PENDING);
		this->sendKey->redraw();
	}
}


void VkvmControl::onPasteComplete(SvgButton * /* tool */) {
	Fl::awake([](void * obj) {
		if (obj == NULL) return;
		VkvmControl * self = static_cast<VkvmControl *>(obj);
		if (self->sendKey == NULL || self->sendKeyDropDown == NULL) return;
		self->sendKey->labelcolor(self->serialSend->inProgress() ? SERIAL_COLOR_PASTE_PENDING : SERIAL_COLOR_PASTE_COMPLETE);
		self->sendKey->redraw();
	}, this);
}


void VkvmControl::onLicense(SvgButton * /* tool */) {
	this->licenseWin->show();
}


void VkvmControl::onVideoResize(VkvmView * /* view */) {
	if (this->video != NULL) {
		char buf[128];
		snprintf(buf, sizeof(buf), "Opened video source with %ux%upx output.",
			unsigned(this->video->captureWidth()), unsigned(this->video->captureHeight())
		);
		this->setStatusLine(buf, true);
	}
	/* update window size according to new aspect ratios */
	this->onFixWindowSize(NULL);
}


void VkvmControl::onVideoClick(VkvmView * /* view */) {
	if (this->video != NULL && Fl::event_button() == FL_LEFT_MOUSE) {
		this->startInputCapture();
	}
}


void VkvmControl::onStatusClick(Fl_Box * status) {
	if (status != NULL && this->statusHistory != NULL && Fl::event_button() == FL_RIGHT_MOUSE) {
		this->statusHistory->show(this->x() + status->x(), this->y() + status->y() + status->h(), status->w());
	}
}


void VkvmControl::onQuit(Fl_Window * /* w */) {
	if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) return;
	this->hide();
}


void VkvmControl::onCaptureDeviceArrival(const char * device) {
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onCaptureDeviceChange(); }, this);
}


void VkvmControl::onCaptureDeviceRemoval(const char * device) {
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onCaptureDeviceChange(); }, this);
}


void VkvmControl::onCaptureDeviceChange() {
	this->videoDevices = this->videoSource.getDeviceList();
	std::sort(
		this->videoDevices.begin(),
		this->videoDevices.end(),
		[] (pcf::video::CaptureDevice * lhs, pcf::video::CaptureDevice * rhs) {
			const int res = ncs_cmpi(lhs->getName(), rhs->getName());
			if (res != 0) return res < 0;
			return ncs_cmpi(lhs->getPath(), rhs->getPath()) < 0;
		}
	);
	if (this->sourceList == NULL || this->video == NULL) return;
	pcf::video::CaptureDevice * lastDevice = this->video->captureDevice();
	const char * lastDevicePath = (lastDevice != NULL) ? lastDevice->getPath() : NULL;
	int selectedIndex = -1;
	/* create drop down list */
	this->sourceList->clear();
	this->sourceList->add("Video Source", 0, PCF_GUI_CALLBACK(onVideoSource), this, FL_MENU_DIVIDER);
	for (size_t n = 0; n < this->videoDevices.size(); n++) {
		pcf::video::CaptureDevice * dev = this->videoDevices[n];
		const char * path = dev->getPath();
		const char * name = dev->getName();
		if (name != NULL) {
			this->sourceList->add(name, 0, PCF_GUI_CALLBACK(onVideoSource), this, 0);
		}
		if (lastDevicePath != NULL && path != NULL && strcmp(lastDevicePath, path) == 0) {
			selectedIndex = int(this->sourceList->size() - 1);
		}
	}
	/* select current video source item in source list and viewer widget */
	if (lastDevice == NULL || selectedIndex == -1) {
		/* no device selected for output */
		if (lastDevice != NULL) {
			this->setStatusLine("Selected video source was removed.");
		}
		this->sourceList->value(0);
		this->video->captureDevice(NULL);
		this->onCaptureViewChange();
		this->stopInputCapture();
	} else {
		/* select previously selected device */
		this->sourceList->value(selectedIndex);
	}
}


void VkvmControl::onCaptureViewChange() {
	if (this->video == NULL) return;
	const bool isCapDevSelected = this->video->captureDevice() != NULL;
	if (this->videoConfig != NULL) {
		if ( isCapDevSelected ) {
			this->videoConfig->activate();
		} else {
			this->videoConfig->deactivate();
		}
		this->videoConfig->redraw();
	}
	if (this->aspectRatio != NULL) {
		if ( isCapDevSelected ) {
			this->aspectRatio->activate();
		} else {
			this->aspectRatio->deactivate();
		}
		this->aspectRatio->redraw();
	}
	if ( isCapDevSelected ) {
		this->video->show();
	} else {
		this->video->hide();
	}
}


void VkvmControl::onSerialPortArrival(const char * /* port */) {
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onSerialPortChange(); }, this);
}


void VkvmControl::onSerialPortRemoval(const char * /* port */) {
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onSerialPortChange(); }, this);
}


void VkvmControl::onSerialPortChange() {
	this->serialPorts = this->serialPortSource.getSerialPortList();
	std::sort(
		this->serialPorts.begin(),
		this->serialPorts.end(),
		[] (const pcf::serial::SerialPort & lhs, const pcf::serial::SerialPort & rhs) {
			return ncs_cmpi(lhs.getPath(), rhs.getPath()) < 0;
		}
	);
	if (this->serialList == NULL) return;
	const char * lastSerialPortPath = this->serialPort.getPath();
	int selectedIndex = 0;
	/* create drop down list */
	this->serialList->clear();
	this->serialList->add("Serial Port", 0, PCF_GUI_CALLBACK(onSerialSource), this, FL_MENU_DIVIDER);
	for (size_t n = 0; n < this->serialPorts.size(); n++) {
		const pcf::serial::SerialPort & port = this->serialPorts[n];
		const char * path = port.getPath();
		const char * name = port.getName();
		if (path != NULL) {
			if (name != NULL) {
				const size_t labelLen = strlen(path) + strlen(name) + 4;
				char * labelStr = static_cast<char *>(malloc(labelLen * sizeof(char)));
				if (labelStr != NULL) {
					snprintf(labelStr, labelLen, "%s - %s", path, name);
					labelStr[labelLen - 1] = 0;
					this->serialList->add(labelStr, 0, PCF_GUI_CALLBACK(onSerialSource), this, 0);
					free(labelStr);
				} else {
					this->serialList->add(path, 0, PCF_GUI_CALLBACK(onSerialSource), this, 0);
				}
			} else {
				this->serialList->add(path, 0, PCF_GUI_CALLBACK(onSerialSource), this, 0);
			}
		}
		if (lastSerialPortPath != NULL && path != NULL && strcmp(lastSerialPortPath, path) == 0) {
			selectedIndex = int(this->serialList->size() - 1);
		}
	}
	/* select currently selected serial port source item in serial list */
	if (lastSerialPortPath == NULL || selectedIndex == 0) {
		/* no serial port selected for output */
		this->serialList->value(0);
		this->disconnectPeriphery();
		if (this->statusConnection != NULL) this->statusConnection->deactivate();
	} else {
		/* select previously selected serial port */
		this->serialList->value(selectedIndex);
	}
}


void VkvmControl::onSerialConnectionChange() {
	if (this->statusConnection == NULL) return;
	const bool isOpen = this->serialDevice.isOpen();
	const bool isConnected = this->serialDevice.isConnected();
	const bool isFullyConnected = this->serialDevice.isFullyConnected();
	this->statusConnection->label(isOpen ? connectedSvg : disconnectedSvg);
	this->statusConnection->selection_color(
		isFullyConnected ? SERIAL_COLOR_FULLY_CONNECTED
		: isConnected ? SERIAL_COLOR_CONNECTED
		: isOpen ? SERIAL_COLOR_PENDING
		: SERIAL_COLOR_DISCONNECTED
	);
	this->statusConnection->redraw();
	if (this->sendKey == NULL) return;
	const bool oldSendKeyState = this->sendKey->active();
	if ( isConnected ) {
		this->sendKey->activate();
	} else {
		this->sendKey->deactivate();
	}
	if (oldSendKeyState != this->sendKey->active()) this->sendKey->redraw();
	this->onKeyboardLedChange();
}


void VkvmControl::onKeyboardLedChange() {
	if (this->statusConnection == NULL || this->statusNumLock == NULL || this->statusCapsLock == NULL || this->statusScrollLock == NULL) return;
	const bool isConnected = this->serialDevice.isConnected();
	const uint8_t leds = this->serialDevice.keyboardLeds();
	if ( isConnected ) {
		this->statusNumLock->activate();
		this->statusNumLock->selection_color(((leds & USBLED_NUM_LOCK) != 0) ? STATUS_COLOR_LED_ON : STATUS_COLOR_LED_OFF);
		this->statusCapsLock->activate();
		this->statusCapsLock->selection_color(((leds & USBLED_CAPS_LOCK) != 0) ? STATUS_COLOR_LED_ON : STATUS_COLOR_LED_OFF);
		this->statusScrollLock->activate();
		this->statusScrollLock->selection_color(((leds & USBLED_SCROLL_LOCK) != 0) ? STATUS_COLOR_LED_ON : STATUS_COLOR_LED_OFF);
	} else {
		this->statusNumLock->deactivate();
		this->statusNumLock->selection_color(STATUS_COLOR_LED_OFF);
		this->statusCapsLock->deactivate();
		this->statusCapsLock->selection_color(STATUS_COLOR_LED_OFF);
		this->statusScrollLock->deactivate();
		this->statusScrollLock->selection_color(STATUS_COLOR_LED_OFF);
	}
	this->statusNumLock->redraw();
	this->statusCapsLock->redraw();
	this->statusScrollLock->redraw();
}


void VkvmControl::onVkvmUsbState(const PeripheryResult res, const uint8_t /* usb */) {
	if (res != PeripheryResult::PR_OK) return;
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onSerialConnectionChange(); }, this);
}


void VkvmControl::onVkvmKeyboardLeds(const PeripheryResult res, const uint8_t /* leds */) {
	if (res != PeripheryResult::PR_OK) return;
	Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->onKeyboardLedChange(); }, this);
}


uint8_t VkvmControl::onVkvmRemapKey(const uint8_t key, const int osKey, const RemapFor action) {
	int val = 0;
	switch (key) {
	case USBKEY_RIGHT_CONTROL: val = 1; break;
	case USBKEY_RIGHT_SHIFT: val = 2; break;
	default: return key;
	}

	bool triggered = false;
	switch (action) {
	case RemapFor::RF_DOWN:
		this->shiftCtrl |= val;
		break;
	case RemapFor::RF_PUSH:
		if ((this->shiftCtrl | val) == 3) triggered = true;
		/* fall-through */
	case RemapFor::RF_UP:
		this->shiftCtrl &= ~val;
		break;
	}
	if (triggered || this->shiftCtrl == 3) {
		/* RSHIFT+RCTRL pressed -> stop input capture */
		Fl::awake([](void * obj) { if (obj != NULL) static_cast<VkvmControl *>(obj)->stopInputCapture(); }, this);
	}

	return key;
}


void VkvmControl::onVkvmConnected() {
	Fl::awake([](void * obj) {
		if (obj == NULL) return;
		VkvmControl * self = static_cast<VkvmControl *>(obj);
		self->onSerialConnectionChange();
		if (self->video != NULL && self->video->captureDevice() != NULL) {
			self->setStatusLine("Connected to periphery device.");
		} else {
			self->setStatusLine("Connected to periphery device. Press CTRL-K to take control in screen-less mode.");
		}
	}, this);
}


void VkvmControl::onVkvmDisconnected(const DisconnectReason reason) {
	this->lastReason = reason;
	Fl::awake([](void * obj) {
		if (obj == NULL) return;
		VkvmControl * self = static_cast<VkvmControl *>(obj);
		self->onSerialConnectionChange();
		switch (self->lastReason) {
		case DisconnectReason::D_USER:
			self->onSerialConnectionChange();
			break;
		case DisconnectReason::D_RECV_ERROR:
			self->setStatusLine("Failed to receive data from serial device.");
			self->onSerialConnectionChange();
			break;
		case DisconnectReason::D_SEND_ERROR:
			self->setStatusLine("Failed to send data to serial device.");
			self->onSerialConnectionChange();
			break;
		case DisconnectReason::D_TIMEOUT:
			if ( self->serialOn ) {
				self->connectPeriphery(); /* retry */
				return;
			}
			break;
		case DisconnectReason::D_INVALID_PROTOCOL:
			self->setStatusLine("Connected serial device uses an incompatible protocol.");
			self->onSerialConnectionChange();
			break;
		default:
			self->setStatusLine();
			break;
		}
		if (self->serialSend != NULL) self->serialSend->stop();
		self->stopInputCapture();
		self->lastReason = DisconnectReason::COUNT;
		if ( ! self->serialChange ) {
			self->serialOn = false;
			if (self->serialList != NULL) self->serialList->value(0);
		}
	}, this);
}


void VkvmControl::setRotation(const VkvmView::Rotation val) {
	if (this->video == NULL || this->rotation == NULL) return;
	this->video->rotation(val);
	this->rotation->label(getRotationSvg(this->video->rotation()));
	this->rotation->redraw();
}


bool VkvmControl::setStatusLine(const char * text, const bool copy) {
	if (this->status1 == NULL) return false;
	if (this->statusHistory != NULL && text != NULL) {
		this->statusHistory->addStatusLine(text, copy);
	}
	if (copy && text != NULL) {
		this->status1->copy_label(text);
	} else {
		this->status1->label((text != NULL) ? text : "");
	}
	return true;
}


void VkvmControl::connectPeriphery() {
	/* delay state display to avoid race condition with event handler */
	Fl::awake([](void * obj) {
		if (obj == NULL) return;
		VkvmControl * self = static_cast<VkvmControl *>(obj);
		const bool isOpen = self->serialDevice.isOpen();
		if (isOpen || self->serialList == NULL) return;
		/* try to connect to keyboard/mouse proxy */
		self->serialChange = false;
		const char * serialPortPath = self->serialPort.getPath();
		if (serialPortPath == NULL) {
			self->setStatusLine("No serial device selected.");
			self->serialOn = false;
			if (self->serialList != NULL) self->serialList->value(0);
		} else if ( ! self->serialDevice.open(*self, serialPortPath) ) {
			self->setStatusLine("Failed to open serial connection. Insufficient permissions?");
			self->serialOn = false;
			if (self->serialList != NULL) self->serialList->value(0);
		} else if (self->lastReason != DisconnectReason::D_TIMEOUT) {
			self->setStatusLine("Connected to serial device. Waiting for periphery device.");
		} else {
			self->setStatusLine("Transmission timed out. Reconnecting to serial device.");
		}
		self->onSerialConnectionChange();
	}, this);
}


void VkvmControl::disconnectPeriphery() {
	/* delay state display to avoid race condition with event handler */
	Fl::awake([](void * obj) {
		if (obj == NULL) return;
		VkvmControl * self = static_cast<VkvmControl *>(obj);
		if (self->serialSend != NULL) self->serialSend->stop();
		self->stopInputCapture();
		if ( ! self->serialChange ) {
			self->serialOn = false;
			if (self->serialList != NULL) self->serialList->value(0);
		}
		self->serialDevice.close();
		self->setStatusLine();
		self->onSerialConnectionChange();
		if ( self->serialChange ) self->connectPeriphery();
	}, this);
}


void VkvmControl::startInputCapture() {
	if (this->redirectInput || ( ! this->serialDevice.isConnected() )) return;
	this->redirectInput = true;
	this->shiftCtrl = 0;
	this->cursor(FL_CURSOR_NONE);
	if (this->serialSend != NULL) this->serialSend->stop();
	if ( this->serialDevice.grabGlobalInput(true) ) {
		/* move mouse to relative position on VkvmView */
		if (this->video != NULL && Fl::belowmouse() == this->video) {
			const int32_t x1 = int32_t(Fl::event_x()) * 0x7FFF / int32_t(this->video->w());
			const int32_t y1 = int32_t(Fl::event_y()) * 0x7FFF / int32_t(this->video->h());
			this->serialDevice.mouseMoveAbs(int16_t(x1), int16_t(y1));
		}
		this->setStatusLine("Release input capture with RIGHT-SHIFT + RIGHT-CTRL.");
	} else {
		this->setStatusLine("Failed to capture keyboard/mouse input.");
	}
}


void VkvmControl::stopInputCapture() {
	if ( ! this->redirectInput ) return;
	this->setStatusLine();
	this->serialDevice.grabGlobalInput(false);
	this->redirectInput = false;
	this->cursor(FL_CURSOR_DEFAULT);
}


} /* namespace gui */
} /* namespace pcf */
