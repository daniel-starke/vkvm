/**
 * @file Vkm.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-10-30
 * @version 2023-11-05
 */
#ifndef __VKM_HPP__
#define __VKM_HPP__

#include <stdint.h>
#include "PluggableUSB.h"
#include "UsbKeys.hpp"
#include "Protocol.hpp"
#include "HidDescriptor.hpp"

#ifdef __AVR__
#include <avr/pgmspace.h>
#define __VKM_HPP__ROM PROGMEM
#define __VKM_HPP__ROM_READ_U8(x, o) pgm_read_byte((x) + (o))
#else /* not __AVR__ */
#define __VKM_HPP__ROM
#define __VKM_HPP__ROM_READ_U8(x, o) (x)[o]
#endif /* not __AVR__ */


/**
 * Defines a little-endian encoded uint8_t value.
 *
 * @param[in] x - uint8_t value to encode
 * @return encoded uint8_t array fields
 */
#define VKM_LE_U8(x) uint8_t((x))


/**
 * Defines a little-endian encoded uint16_t value.
 *
 * @param[in] x - uint16_t value to encode
 * @return encoded uint8_t array fields
 */
#define VKM_LE_U16(x) VKM_LE_U8((x)), VKM_LE_U8((x) >> 8)


/* forward declarations */
class VkmBase;
static inline VkmBase & Vkm();


/**
 * Virtual Keyboard/Mouse handling class based on USB HID.
 */
class VkmBase : protected PluggableUSBModule {
public:
	/* VKM specific constants. */
	enum {
		/* USB HID v1.11 chapter 7.2 */
		VKM_HID_GET_REPORT = 0x01,
		VKM_HID_GET_IDLE = 0x02,
		VKM_HID_GET_PROTOCOL = 0x03,
		VKM_HID_SET_REPORT = 0x09,
		VKM_HID_SET_IDLE = 0x0A,
		VKM_HID_SET_PROTOCOL = 0x0B,
		/* USB HID v1.11 chapter 7.1 */
		VKM_HID_HID_DESCRIPTOR_TYPE = 0x21,
		VKM_HID_REPORT_DESCRIPTOR_TYPE = 0x22,
		VKM_HID_PHYSICAL_DESCRIPTOR_TYPE = 0x23,
		/* USB HID v1.11 chapter 4.2 */
		VKM_HID_SUBCLASS_NONE = 0,
		VKM_HID_SUBCLASS_BOOT_INTERFACE = 1,
		/* USB HID v1.11 chapter 4.3 */
		VKM_HID_PROTOCOL_NONE = 0,
		VKM_HID_PROTOCOL_KEYBOARD = 1,
		VKM_HID_PROTOCOL_MOUSE = 2,
		/* USB HID v1.11 chapter 7.2.1 */
		VKM_HID_REPORT_TYPE_INPUT = 1,
		VKM_HID_REPORT_TYPE_OUTPUT = 2,
		VKM_HID_REPORT_TYPE_FEATURE = 3,
		/* USB HID v1.11 chapter 7.2.5 */
		VKM_HID_BOOT_PROTOCOL = 0,
		VKM_HID_REPORT_PROTOCOL	= 1,
		/* endpoint offset/interface index */
		VKM_IDX_KEYBOARD = 0, /**< Endpoint and interface index for keyboard reports. */
		VKM_IDX_REL_MOUSE = 1, /**< Endpoint and interface index for relative mouse reports. */
		VKM_IDX_ABS_MOUSE = 2, /**< Endpoint and interface index for absolute mouse reports. */
		/* report IDs */
		VKM_ID_KEYBOARD = 0, /**< Report ID for keyboard reports (not used). */
		VKM_ID_REL_MOUSE = 1, /**< Report ID for relative mouse reports. */
		VKM_ID_ABS_MOUSE = 2 /**< Report ID for absolute mouse reports. */
	};
	/** Keyboard HID report structure. */
	struct KeyReport {
		uint8_t modifiers; /**< Modifier key bits. E.g. `USBWRITE_LEFT_CONTROL`. */
		uint8_t oem; /* unused */
		uint8_t keys[6]; /**< List of currently pressed key codes. E.g. `USBKEY_A`. */
		/**
		 * Constructor.
		 *
		 * @param[in] m - key modifier bits
		 * @param[in] k0 - pressed key 0
		 * @param[in] k1 - pressed key 1
		 * @param[in] k2 - pressed key 2
		 * @param[in] k3 - pressed key 3
		 * @param[in] k4 - pressed key 4
		 * @param[in] k5 - pressed key 5
		 */
		explicit inline KeyReport(
			const uint8_t m = 0,
			const uint8_t k0 = USBKEY_NO_EVENT, const uint8_t k1 = USBKEY_NO_EVENT,
			const uint8_t k2 = USBKEY_NO_EVENT, const uint8_t k3 = USBKEY_NO_EVENT,
			const uint8_t k4 = USBKEY_NO_EVENT, const uint8_t k5 = USBKEY_NO_EVENT
		):
			modifiers{m},
			oem{0},
			keys{k0, k1, k2, k3, k4, k5}
		{}
	};
	/** Relative mouse HID report structure. */
	struct RelMouseReport {
		uint8_t reportId; /**< Report ID. */
		uint8_t buttons; /**< Bits of the pressed buttons. E.g. `USBBUTTON_LEFT`. */
		int8_t x; /**< Relative displacement in x direction. */
		int8_t y; /**< Relative displacement in y direction. */
		int8_t wheel; /**< Wheel movement ticks. */
		/**
		 * Constructor.
		 *
		 * @param[in] b - pressed button bits
		 * @param[in] aX - x displacement
		 * @param[in] aY - y displacement
		 * @param[in] w - wheel movement ticks
		 */
		explicit inline RelMouseReport(const uint8_t b, const int8_t aX = 0, const int8_t aY = 0, const int8_t w = 0):
			reportId{uint8_t(VKM_ID_REL_MOUSE)},
			buttons{b},
			x{aX},
			y{aY},
			wheel{w}
		{}
	};
	/** Absolute mouse HID report structure. */
	struct AbsMouseReport {
		uint8_t reportId; /**< Report ID. */
		uint8_t buttons; /* unused */
		uint8_t xLow; /**< Lower byte of the absolute screen x coordinate. */
		uint8_t xHigh; /**< Higher byte of the absolute screen x coordinate. */
		uint8_t yLow; /**< Lower byte of the absolute screen y coordinate. */
		uint8_t yHigh; /**< Higher byte of the absolute screen y coordinate. */
		int8_t wheel; /* unused */
		/**
		 * Constructor.
		 *
		 * @param[in] x - absolute screen x coordinate (0..32767)
		 * @param[in] y - absolute screen y coordinate (0..32767)
		 */
		explicit inline AbsMouseReport(const int16_t x, const int16_t y):
			reportId{VKM_ID_ABS_MOUSE},
			buttons{0},
			xLow{uint8_t(x & 0xFF)},
			xHigh{uint8_t((x >> 8) & 0x7F)},
			yLow{uint8_t(y & 0xFF)},
			yHigh{uint8_t((y >> 8) & 0x7F)},
			wheel{0}
		{}
	};
private:
	/* USB HID v1.11 chapter 6.2.1 */
	struct HidDescriptor {
		uint8_t bLength;
		uint8_t bDescriptorType;
		uint8_t bcdHidLow;
		uint8_t bcdHidHigh;
		uint8_t bCountryCode;
		uint8_t bNumDescriptors;
		uint8_t bClassDescriptorType;
		uint8_t wClassDescriptorLengthLow;
		uint8_t wClassDescriptorLengthHigh;
	};
	/* USB HID v1.11 chapter 5.1 */
	struct Descriptor {
		InterfaceDescriptor dev;
		HidDescriptor hid;
		EndpointDescriptor in;
	};

	uint8_t epType[3]; /**< Endpoint types. */
	uint8_t protocolKeyboard; /**< Boot/report mode. */
	uint8_t protocolRelMouse; /**< Boot/report mode. */
	uint8_t protocolAbsMouse; /**< Boot/report mode. */
	uint8_t idleKeyboard; /**< USB idle mode flag. */
	uint8_t idleRelMouse; /**< USB idle mode flag. */
	uint8_t idleAbsMouse; /**< USB idle mode flag. */
	uint8_t buttons; /**< Mouse buttons currently pressed. */
	volatile uint8_t leds; /**< Keyboard LED states. */
	KeyReport keyReport; /**< Key report to set and send. */
protected:
	/** USB HID descriptor data. */
	struct DescriptorData {
		const void * ptr; /**< PGM pointer. */
		size_t len; /**< Number of bytes. */
	};
public:
	/** Constructor. */
	explicit VkmBase():
		PluggableUSBModule{3, 3, epType},
		epType{EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_IN, EP_TYPE_INTERRUPT_IN}, /* device to host endpoints */
		protocolKeyboard{VKM_HID_REPORT_PROTOCOL},
		protocolRelMouse{VKM_HID_REPORT_PROTOCOL},
		protocolAbsMouse{VKM_HID_REPORT_PROTOCOL},
		idleKeyboard{1},
		idleRelMouse{1},
		idleAbsMouse{1},
		buttons{0},
		leds{0},
		keyReport{}
	{
		PluggableUSB().plug(this);
	}

	/**
	 * Needed to have the constructor called.
	 */
	void begin() {}

	/**
	 * Returns a bit field with the current states of the keyboard LEDs.
	 *
	 * @return keyboard LED states
	 */
	uint8_t getLeds() const {
		return this->leds;
	}

	/**
	 * Presses the given key.
	 *
	 * @param[in] key - key to press
	 * @return true on success, else false
	 */
	bool pressKey(const uint8_t key) {
		switch (key) {
		case USBKEY_LEFT_CONTROL:  /* 0xE0 */
		case USBKEY_LEFT_SHIFT:    /* 0xE1 */
		case USBKEY_LEFT_ALT:      /* 0xE2 */
		case USBKEY_LEFT_GUI:      /* 0xE3 */
		case USBKEY_RIGHT_CONTROL: /* 0xE4 */
		case USBKEY_RIGHT_SHIFT:   /* 0xE5 */
		case USBKEY_RIGHT_ALT:     /* 0xE6 */
		case USBKEY_RIGHT_GUI:     /* 0xE7 */
			this->keyReport.modifiers = uint8_t(this->keyReport.modifiers | (1 << (key - 0xE0)));
			return this->sendReport(this->keyReport);
		default:
			for (uint8_t i = 0; i < 6; i++) {
				if (this->keyReport.keys[i] == key) return 0; /* avoid duplicates */
			}
			for (uint8_t i = 0; i < 6; i++) {
				if (this->keyReport.keys[i] == USBKEY_NO_EVENT) {
					this->keyReport.keys[i] = key;
					return this->sendReport(this->keyReport);
				}
			}
			break;
		}
		return false;
	}

	/**
	 * Releases the given key.
	 *
	 * @param[in] key - key to release
	 * @return true on success, else false
	 */
	bool releaseKey(const uint8_t key) {
		switch (key) {
		case USBKEY_LEFT_CONTROL:  /* 0xE0 */
		case USBKEY_LEFT_SHIFT:    /* 0xE1 */
		case USBKEY_LEFT_ALT:      /* 0xE2 */
		case USBKEY_LEFT_GUI:      /* 0xE3 */
		case USBKEY_RIGHT_CONTROL: /* 0xE4 */
		case USBKEY_RIGHT_SHIFT:   /* 0xE5 */
		case USBKEY_RIGHT_ALT:     /* 0xE6 */
		case USBKEY_RIGHT_GUI:     /* 0xE7 */
			this->keyReport.modifiers = uint8_t(this->keyReport.modifiers & ~(1 << (key - 0xE0)));
			return this->sendReport(this->keyReport);
			break;
		default:
			for (uint8_t i = 0; i < 6; i++) {
				if (this->keyReport.keys[i] == key) {
					this->keyReport.keys[i] = USBKEY_NO_EVENT;
					return this->sendReport(this->keyReport);
				}
			}
			break;
		}
		return false;
	}

	/**
	 * Releases all pressed keys.
	 */
	bool releaseAllKeys() {
		this->keyReport = KeyReport{};
		return this->sendReport(this->keyReport);
	}

	/**
	 * Pushes the given key once.
	 *
	 * @param[in] key - key to push
	 * @return true on success, else false
	 */
	bool pushKey(const uint8_t key) {
		const size_t res = this->pressKey(key) ? 1 : 0;
		this->releaseKey(key);
		return res;
	}

	/**
	 * Pushes the given keys once.
	 *
	 * @param[in] buffer - keys to push
	 * @param[in] size - number of keys to push
	 * @return number of keys pushed
	 */
	size_t pushKeys(const uint8_t * buffer, const size_t size) {
		size_t res = 0;
		for (; res < size; res++) {
			if ( ! this->pushKey(buffer[res]) ) break;
		}
		return res;
	}

	/**
	 * Writes the given keys to the host while applying the given modifiers. All pressed keys are
	 * released at the beginning. Key modifiers found will toggle their state instead of being sent
	 * as single event to the host.
	 *
	 * @param[in] mod - keyboard modifiers (e.g. USBWRITE_RIGHT_NUM_LOCK)
	 * @param[in] buffer - keys to write
	 * @param[in] size - number of keys to write
	 * @return number of keys written
	 */
	size_t write(const uint8_t mod, const uint8_t * buffer, const size_t size) {
		size_t res = 0;
		uint8_t oldLeds = this->leds;
		uint8_t ledsToggled = 0;
		bool revertNumLock = false;
		bool revertKana = false;
		/* release all keys */
		if ( ! this->releaseAllKeys() ) return 0;
		/* apply keyboard modifiers */
		if (((mod & USBWRITE_RIGHT_NUM_LOCK) != 0) != ((oldLeds & USBLED_NUM_LOCK) != 0)) {
			if ( ! this->pushKey(USBKEY_NUM_LOCK) ) return 0;
			ledsToggled |= uint8_t(USBLED_NUM_LOCK);
			revertNumLock = true;
		}
		if (((mod & USBWRITE_RIGHT_KANA) != 0) != ((oldLeds & USBLED_KANA) != 0)) {
			if ( ! this->pushKey(USBKEY_IME_KANA) ) return 0;
			ledsToggled |= uint8_t(USBLED_KANA);
			revertKana = true;
		}
		if ((mod & USBWRITE_LEFT_CONTROL)  != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_LEFT_CONTROL  - 0xE0));
		if ((mod & USBWRITE_LEFT_SHIFT)    != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_LEFT_SHIFT    - 0xE0));
		if ((mod & USBWRITE_LEFT_ALT)      != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_LEFT_ALT      - 0xE0));
		if ((mod & USBWRITE_RIGHT_CONTROL) != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_RIGHT_CONTROL - 0xE0));
		if ((mod & USBWRITE_RIGHT_SHIFT)   != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_RIGHT_SHIFT   - 0xE0));
		if ((mod & USBWRITE_RIGHT_ALT)     != 0) this->keyReport.modifiers |= uint8_t(1 << (USBKEY_RIGHT_ALT     - 0xE0));
		if ( ! this->waitForLedsToggled(oldLeds, ledsToggled) ) return false;
		oldLeds = this->leds;
		/* push keys */
		for (size_t i = 0; i < size; i++) {
			const uint8_t key = buffer[i];
			switch (key) {
			case USBKEY_LEFT_CONTROL:  /* 0xE0 */
			case USBKEY_LEFT_SHIFT:    /* 0xE1 */
			case USBKEY_LEFT_ALT:      /* 0xE2 */
			case USBKEY_LEFT_GUI:      /* 0xE3 */
			case USBKEY_RIGHT_CONTROL: /* 0xE4 */
			case USBKEY_RIGHT_SHIFT:   /* 0xE5 */
			case USBKEY_RIGHT_ALT:     /* 0xE6 */
			case USBKEY_RIGHT_GUI:     /* 0xE7 */
				this->keyReport.modifiers ^= uint8_t(1 << (key - 0xE0));
				break;
			default:
				if ( ! this->pushKey(key) ) goto endOfLoop;
				res++;
				break;
			}
		} endOfLoop:
		/* revert keyboard modifiers */
		this->keyReport.modifiers = 0;
		this->sendReport(this->keyReport);
		if ( revertKana ) this->pushKey(USBKEY_IME_KANA);
		if ( revertNumLock ) this->pushKey(USBKEY_NUM_LOCK);
		this->waitForLedsToggled(oldLeds, ledsToggled);
		return res;
	}

	/**
	 * Pushes the given mouse button.
	 *
	 * @param[in] button - button to click
	 * @return true on success, else false
	 */
	bool pushButton(const uint8_t button) {
		const bool res = this->pressButton(button);
		this->releaseButton(button);
		return res;
	}

	/**
	 * Presses the given mouse button.
	 *
	 * @param[in] button - button to press
	 * @return true on success, else false
	 */
	bool pressButton(const uint8_t button) {
		this->buttons = uint8_t(this->buttons | (button & USBBUTTON_ALL));
		return this->sendReport(RelMouseReport{this->buttons});
	}

	/**
	 * Releases the given mouse button.
	 *
	 * @param[in] button - button to release
	 * @return true on success, else false
	 */
	bool releaseButton(const uint8_t button) {
		this->buttons = uint8_t(this->buttons & ~(button & USBBUTTON_ALL));
		return this->sendReport(RelMouseReport{this->buttons});
	}

	/**
	 * Moves the mouse pointer by the given relative values on the defined axises.
	 *
	 * @param[in] x - delta on the x axis
	 * @param[in] y - delta on the y axis
	 * @return true on success, else false
	 */
	bool moveRel(const int8_t x, const int8_t y) {
		return this->sendReport(RelMouseReport{this->buttons, x, y, 0});
	}

	/**
	 * Turns the mouse wheel.
	 *
	 * @param[in] wheel - mouse wheel delta
	 * @return true on success, else false
	 */
	bool scroll(const int8_t wheel) {
		return this->sendReport(RelMouseReport{this->buttons, 0, 0, wheel});
	}

	/**
	 * Moves the mouse pointer by the given absolute values on the defined axises.
	 *
	 * @param[in] x - on the x axis
	 * @param[in] y - on the y axis
	 * @return true on success, else false
	 */
	bool moveAbs(const int16_t x, const int16_t y) {
		return this->sendReport(AbsMouseReport{x, y});
	}
protected:
	/**
	 * Returns the HID descriptor for the boot keyboard stored in flash.
	 *
	 * @return keyboard HID descriptor
	 */
	static DescriptorData getKeyboardDesc() {
		constexpr static const auto descSrc = hid::fromSource(R"(
			# Keyboard
			UsagePage(GenericDesktop)
			Usage(Keyboard)
			Collection(Application)
				UsagePage(Keyboard)
				UsageMinimum(KeyboardLeftControl)
				UsageMaximum(KeyboardRightGui)
				LogicalMinimum(0)
				LogicalMaximum(1)
				ReportSize(1)
				ReportCount(8)
				Input(Data, Var, Abs)
				ReportSize(1)
				ReportCount(8)
				UsagePage(Led)
				UsageMinimum(NumLock)
				UsageMaximum(DoNotDisturb)
				ReportSize(1)
				ReportCount(8)
				Output(Data, Var, Abs)
				Input(Cnst, Var, Abs)
				ReportSize(8)
				ReportCount(6)
				LogicalMinimum(0)
				LogicalMaximum(255)
				UsagePage(Keyboard)
				UsageMinimum(NoEventIndicated) # 0x00
				UsageMaximum(KeyboardLang2)    # 0x91
				Input(Data, Ary, Abs)
			EndCollection
			)")
		;
		constexpr static const hid::Error error = hid::compileError(descSrc);
		constexpr static const size_t dummy = hid::reporter<error.line, error.column, error.message>();
		constexpr static const auto desc __VKM_HPP__ROM = hid::Descriptor<hid::compiledSize(descSrc)>(descSrc);
		(void)dummy; /* silence unused warning */
		return DescriptorData{desc.data, desc.size()};
	}

	/**
	 * Returns the HID descriptor for the boot mouse (relative) stored in flash.
	 *
	 * @return mouse HID descriptor
	 */
	static DescriptorData getRelMouseDesc() {
		constexpr static const auto descSrc = hid::fromSource(R"(
			# Relative Mouse
			UsagePage(GenericDesktop)
			Usage(Mouse)
			Collection(Application)
				UsagePage(GenericDesktop)
				Usage(Pointer)
				Collection(Physical)
					ReportId({RelMouseId})
					UsagePage(Button)
					UsageMinimum(Button1)
					UsageMaximum(Button3)
					LogicalMinimum(0)
					LogicalMaximum(1)
					ReportSize(1)
					ReportCount(3)
					Input(Data, Var, Abs)
					ReportSize(5)
					ReportCount(1)
					Input(Cnst, Var, Abs)
					UsagePage(GenericDesktop)
					Usage(X)
					Usage(Y)
					Usage(Wheel)
					LogicalMinimum(-127)
					LogicalMaximum(127)
					ReportSize(8)
					ReportCount(3)
					Input(Data, Var, Rel)
				EndCollection
			EndCollection
			)")
			("RelMouseId", VKM_ID_REL_MOUSE)
		;
		constexpr static const hid::Error error = hid::compileError(descSrc);
		constexpr static const size_t dummy = hid::reporter<error.line, error.column, error.message>();
		constexpr static const auto desc __VKM_HPP__ROM = hid::Descriptor<hid::compiledSize(descSrc)>(descSrc);
		(void)dummy; /* silence unused warning */
		return DescriptorData{desc.data, desc.size()};
	}

	/**
	 * Returns the HID descriptor for the mouse (absolute) stored in flash.
	 *
	 * @return mouse HID descriptor
	 */
	static DescriptorData getAbsMouseDesc() {
		constexpr static const auto descSrc = hid::fromSource(R"(
			# Absolute Mouse
			UsagePage(GenericDesktop)
			Usage(Mouse)
			Collection(Application)
				UsagePage(GenericDesktop)
				Usage(Pointer)
				Collection(Physical)
					ReportId({AbsMouseId})
					UsagePage(Button)
					UsageMinimum(Button1)
					UsageMaximum(Button3)
					LogicalMinimum(0)
					LogicalMaximum(1)
					ReportSize(1)
					ReportCount(3)
					Input(Data, Var, Abs)
					ReportSize(5)
					ReportCount(1)
					Input(Cnst, Var, Abs)
					UsagePage(GenericDesktop)
					Usage(X)
					Usage(Y)
					LogicalMinimum(0)
					LogicalMaximum(32767)
					ReportSize(16)
					ReportCount(2)
					Input(Data, Var, Abs)
					UsagePage(GenericDesktop)
					Usage(Wheel)
					LogicalMinimum(-127)
					LogicalMaximum(127)
					ReportSize(8)
					ReportCount(1)
					Input(Data, Var, Rel)
				EndCollection
			EndCollection
			)")
			("AbsMouseId", VKM_ID_ABS_MOUSE)
		;
		constexpr static const hid::Error error = hid::compileError(descSrc);
		constexpr static const size_t dummy = hid::reporter<error.line, error.column, error.message>();
		constexpr static const auto desc __VKM_HPP__ROM = hid::Descriptor<hid::compiledSize(descSrc)>(descSrc);
		(void)dummy; /* silence unused warning */
		return DescriptorData{desc.data, desc.size()};
	}

	/**
	 * Sends the USB interface description to the host.
	 *
	 * @param[in,out] interfaceCount - index of this interface
	 * @return bytes sent
	 * @see USB HID v1.11 Appendix B for boot interface
	 */
	virtual int getInterface(uint8_t * interfaceCount) {
		*interfaceCount = uint8_t(*interfaceCount + 3); /* uses 3 */
		const size_t keyboardDescSize = VkmBase::getKeyboardDesc().len;
		const size_t relMouseDescSize = VkmBase::getRelMouseDesc().len;
		const size_t absMouseDescSize = VkmBase::getAbsMouseDesc().len;
		const Descriptor hidInterface[3] = {
			/* keyboard */
			{
				D_INTERFACE(uint8_t(this->pluggedInterface + VKM_IDX_KEYBOARD), 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, VKM_HID_SUBCLASS_BOOT_INTERFACE, VKM_HID_PROTOCOL_KEYBOARD),
				/* USB HID v1.11 chapter E.4 */
				{
					VKM_LE_U8(sizeof(HidDescriptor)),
					VKM_LE_U8(VKM_HID_HID_DESCRIPTOR_TYPE), /* descriptor type */
					VKM_LE_U16(0x0111), /* HID version */
					VKM_LE_U8(0), /* not localized */
					VKM_LE_U8(1), /* number of HID class descriptors */
					VKM_LE_U8(VKM_HID_REPORT_DESCRIPTOR_TYPE),
					VKM_LE_U16(keyboardDescSize)
				},
				D_ENDPOINT(USB_ENDPOINT_IN(uint8_t(this->pluggedEndpoint + VKM_IDX_KEYBOARD)), USB_ENDPOINT_TYPE_INTERRUPT, 8 /* USB_EP_SIZE */, 4 /* ms poll interval */)
			/* relative mouse */
			}, {
				D_INTERFACE(uint8_t(this->pluggedInterface + VKM_IDX_REL_MOUSE), 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, VKM_HID_SUBCLASS_BOOT_INTERFACE, VKM_HID_PROTOCOL_MOUSE),
				/* USB HID v1.11 chapter E.4 */
				{
					VKM_LE_U8(sizeof(HidDescriptor)),
					VKM_LE_U8(VKM_HID_HID_DESCRIPTOR_TYPE), /* descriptor type */
					VKM_LE_U16(0x0111), /* HID version */
					VKM_LE_U8(0), /* not localized */
					VKM_LE_U8(1), /* number of HID class descriptors */
					VKM_LE_U8(VKM_HID_REPORT_DESCRIPTOR_TYPE),
					VKM_LE_U16(relMouseDescSize)
				},
				D_ENDPOINT(USB_ENDPOINT_IN(uint8_t(this->pluggedEndpoint + VKM_IDX_REL_MOUSE)), USB_ENDPOINT_TYPE_INTERRUPT, 5 /* USB_EP_SIZE */, 1 /* ms poll interval */)
			/* absolute mouse */
			}, {
				D_INTERFACE(uint8_t(this->pluggedInterface + VKM_IDX_ABS_MOUSE), 1, USB_DEVICE_CLASS_HUMAN_INTERFACE, VKM_HID_SUBCLASS_NONE, VKM_HID_PROTOCOL_NONE),
				/* USB HID v1.11 chapter E.4 */
				{
					VKM_LE_U8(sizeof(HidDescriptor)),
					VKM_LE_U8(VKM_HID_HID_DESCRIPTOR_TYPE), /* descriptor type */
					VKM_LE_U16(0x0111), /* HID version */
					VKM_LE_U8(0), /* not localized */
					VKM_LE_U8(1), /* number of HID class descriptors */
					VKM_LE_U8(VKM_HID_REPORT_DESCRIPTOR_TYPE),
					VKM_LE_U16(absMouseDescSize)
				},
				D_ENDPOINT(USB_ENDPOINT_IN(uint8_t(this->pluggedEndpoint + VKM_IDX_ABS_MOUSE)), USB_ENDPOINT_TYPE_INTERRUPT, 7 /* USB_EP_SIZE */, 1 /* ms poll interval */)
			}
		};
		return USB_SendControl(0, &hidInterface, sizeof(hidInterface));
	}

	/**
	 * Sends the interface specific USB HID device descriptor to the host.
	 *
	 * @param[in] setup - USB setup message
	 * @return bytes sent
	 */
	virtual int getDescriptor(USBSetup & setup) {
		static const DescriptorData keyboardDesc = VkmBase::getKeyboardDesc();
		static const DescriptorData relMouseDesc = VkmBase::getRelMouseDesc();
		static const DescriptorData absMouseDesc = VkmBase::getAbsMouseDesc();
		/* only process HID class descriptor requests */
		if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
		if (setup.wValueH != VKM_HID_REPORT_DESCRIPTOR_TYPE) return 0;
		/* check if the interface number is correctly set */
		switch (uint8_t(setup.wIndex - this->pluggedInterface)) {
		case VKM_IDX_KEYBOARD:
			this->protocolKeyboard = VKM_HID_REPORT_PROTOCOL; /* reset boot/report protocol to report on enumeration */
			return USB_SendControl(TRANSFER_PGM | TRANSFER_RELEASE, keyboardDesc.ptr, keyboardDesc.len);
		case VKM_IDX_REL_MOUSE:
			this->protocolRelMouse = VKM_HID_REPORT_PROTOCOL; /* reset boot/report protocol to report on enumeration */
			return USB_SendControl(TRANSFER_PGM | TRANSFER_RELEASE, relMouseDesc.ptr, relMouseDesc.len);
		case VKM_IDX_ABS_MOUSE:
			this->protocolAbsMouse = VKM_HID_REPORT_PROTOCOL; /* reset boot/report protocol to report on enumeration */
			return USB_SendControl(TRANSFER_PGM | TRANSFER_RELEASE, absMouseDesc.ptr, absMouseDesc.len);
		default:
			return 0;
		}
	}

	/**
	 * Returns the USB device serial number.
	 *
	 * @param[out] name - copy serial number to this buffer
	 * @return number of bytes copied
	 */
	virtual uint8_t getShortName(char * name) {
		name[0] = 'V';
		name[1] = 'K';
		name[2] = 'V';
		name[3] = 'M';
		name[4] = '0' + ((VKVM_PROT_VERSION >> 12) & 0x0F);
		name[5] = '0' + ((VKVM_PROT_VERSION >> 8) & 0x0F);
		name[6] = '0' + ((VKVM_PROT_VERSION >> 4) & 0x0F);
		name[7] = '0' + (VKVM_PROT_VERSION & 0x0F);
		return 8;
	}

	/**
	 * USB setup handler.
	 *
	 * @param[in] setup - USB setup message
	 * @return true on success, else false
	 */
	virtual bool setup(USBSetup & setup) {
		const uint8_t idx = uint8_t(setup.wIndex - this->pluggedInterface);
		uint8_t * protocol, * idle;
		switch (idx) {
		case VKM_IDX_KEYBOARD:
			protocol = &(this->protocolKeyboard);
			idle = &(this->idleKeyboard);
			break;
		case VKM_IDX_REL_MOUSE:
			protocol = &(this->protocolRelMouse);
			idle = &(this->idleRelMouse);
			break;
		case VKM_IDX_ABS_MOUSE:
			protocol = &(this->protocolAbsMouse);
			idle = &(this->idleAbsMouse);
			break;
		default:
			return false;
		}
		switch (setup.bmRequestType) {
		case REQUEST_DEVICETOHOST_CLASS_INTERFACE:
			switch (setup.bRequest) {
			case VKM_HID_GET_REPORT:
				return true;
			case VKM_HID_GET_PROTOCOL:
#ifdef __AVR__
				UEDATX = *protocol;
#else
				USB_SendControl(TRANSFER_RELEASE, protocol, 1);
#endif
				return true;
			case VKM_HID_GET_IDLE:
#ifdef __AVR__
				UEDATX = *idle;
#else
				USB_SendControl(TRANSFER_RELEASE, idle, 1);
#endif
				return true;
			default:
				break;
			}
			break;
		case REQUEST_HOSTTODEVICE_CLASS_INTERFACE:
			switch (setup.bRequest) {
			case VKM_HID_SET_REPORT:
				switch (setup.wValueH) {
				case VKM_HID_REPORT_TYPE_OUTPUT:
					if (setup.wValueL == 0 && idx == VKM_IDX_KEYBOARD && setup.wLength == 1) {
						/* without report ID */
						uint8_t newLeds = this->leds;
						USB_RecvControl(&newLeds, 1);
						this->leds = newLeds;
						return true;
					}
					break;
				default:
					break; /* unsupported */
				}
				return true;
			case VKM_HID_SET_PROTOCOL:
				*protocol = setup.wValueL; /* boot (0)/report (1) protocol */
				return true;
			case VKM_HID_SET_IDLE:
				*idle = setup.wValueL; /* report ID, 0 == all */
				return true;
			default:
				break;
			}
			break;
		default:
			break;
		}

		return false;
	}

	/**
	 * Sends the given report to the host.
	 *
	 * @param[in] report - report to send
	 * @return true on success, else false
	 */
	inline bool sendReport(const KeyReport & report) {
		return this->sendReport(uint8_t(this->pluggedEndpoint + VKM_IDX_KEYBOARD), report);
	}

	/**
	 * Sends the given report to the host.
	 *
	 * @param[in] report - report to send
	 * @return true on success, else false
	 */
	inline bool sendReport(const RelMouseReport & report) {
		return this->sendReport(uint8_t(this->pluggedEndpoint + VKM_IDX_REL_MOUSE), report);
	}

	/**
	 * Sends the given report to the host.
	 *
	 * @param[in] report - report to send
	 * @return true on success, else false
	 */
	inline bool sendReport(const AbsMouseReport & report) {
		return this->sendReport(uint8_t(this->pluggedEndpoint + VKM_IDX_ABS_MOUSE), report);
	}
private:
	friend VkmBase & Vkm();
	/**
	 * Returns a singleton instance of this class.
	 *
	 * @return singleton instance
	 */
	static VkmBase & getInstance() {
		static VkmBase singleton;
		return singleton;
	}

	/**
	 * Waits for the LEDs to toggle the given bits.
	 *
	 * @param[in] oldLeds - old LED values
	 * @param[in] toggleBits - bits to wait for
	 * @param[in] timeout - timeout in milliseconds
	 * @return true on successful toggle, else false
	 */
	bool waitForLedsToggled(const uint8_t oldLeds, const uint8_t toggleBits, const unsigned long timeout = 250) {
		if (toggleBits == 0) return true;
		const unsigned long start = millis();
		while ((millis() - start) < timeout) {
			if ((this->leds ^ toggleBits) == oldLeds) return true;
		}
		return false;
	}

	/**
	 * Sends the given report to the host using the passed endpoint.
	 *
	 * @param[in] ep - endpoint to use
	 * @param[in] report - report to send
	 * @return true on success, else false
	 */
	template <typename T>
	inline bool sendReport(const uint8_t ep, const T & report) {
		return USB_Send(ep | TRANSFER_RELEASE, &report, sizeof(T)) == sizeof(T);
	}
};


/**
 * Returns a singleton VkmBase instance.
 *
 * @return singleton VkmBase instance
 */
static inline VkmBase & Vkm() {
	return VkmBase::getInstance();
}


#endif /* __VKM_HPP__ */
