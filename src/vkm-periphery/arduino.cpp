/**
 * @file arduino.ino
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-10-11
 * @version 2023-11-07
 *
 * Virtual Keyboard/Mouse periphery device.
 * Tested with Arduino IDE 1.8.5 and ATmega32U4 (a.k.a. Arduino Pro Micro).
 */
#include <Arduino.h>
#include "arduino.hpp"
#ifdef VKVM_LED_PWM
#include <wiring_irq.h>
#include <wiring_private.h>
#endif /* VKVM_LED_PWM */


Framing<VKVM_MAX_FRAME_SIZE> framing(handleWrite);


const tHandler handler[RequestType::COUNT] VKVM_ROM = {
	getProtocolVersion,
	getAlive,
	getUsbState,
	getKeyboardLeds,
	setKeyboardDown,
	setKeyboardUp,
	setKeyboardAllUp,
	setKeyboardPush,
	setKeyboardWrite,
	setMouseButtonDown,
	setMouseButtonUp,
	setMouseButtonAllUp,
	setMouseButtonPush,
	setMouseMoveAbs,
	setMouseMoveRel,
	setMouseScroll
};


#ifdef DEBUG
const char * frameType[] = {
	"GET_PROTOCOL_VERSION",
	"GET_ALIVE",
	"GET_USB_STATE",
	"GET_KEYBOARD_LEDS",
	"SET_KEYBOARD_DOWN",
	"SET_KEYBOARD_UP",
	"SET_KEYBOARD_ALL_UP",
	"SET_KEYBOARD_PUSH",
	"SET_KEYBOARD_WRITE",
	"SET_MOUSE_BUTTON_DOWN",
	"SET_MOUSE_BUTTON_UP",
	"SET_MOUSE_BUTTON_ALL_UP",
	"SET_MOUSE_BUTTON_PUSH",
	"SET_MOUSE_MOVE_ABS",
	"SET_MOUSE_MOVE_REL",
	"SET_MOUSE_SCROLL"
};
#endif /* DEBUG */


/**
 * Returns the current USB periphery state
 *
 * @return USB periphery state
 */
static uint8_t getCurrentUsbState() {
	uint8_t res = USBSTATE_OFF;
#ifdef PIN_USB2_SENSE
	if ( digitalRead(PIN_USB2_SENSE) ) {
		res = uint8_t(res | USBSTATE_ON);
	}
#endif /* PIN_USB2_SENSE */
	if ( USBDevice.configured() ) {
		res = uint8_t(res | USBSTATE_CONFIGURED);
	}
	return res;
}


/**
 * Processes a received frame.
 *
 * @param[in] seq - frame sequence number
 * @param[in] buf - frame data
 * @param[in] len - frame data length
 * @param[in] err - true on error, else false
 */
void handleFrame(const uint8_t seq, const uint8_t * buf, const size_t len, const bool err) {
	if (len < 1 || err) {
		sendResponse(seq, ResponseType::E_BROKEN_FRAME);
		return;
	}
	if (*buf >= RequestType::COUNT) {
		sendResponse(seq, ResponseType::E_INVALID_REQ_TYPE, *buf);
		return;
	}
	const tHandler fn = (tHandler)(VKVM_ROM_READ_PTR(handler, *buf));
	if (fn == NULL) {
		sendResponse(seq, ResponseType::E_INVALID_REQ_TYPE, *buf);
		return;
	}
	DBG_MSG(frameType[*buf]);
#ifdef PIN_USB2_SENSE
	const bool usbConnected = digitalRead(PIN_USB2_SENSE);
#else /* not PIN_USB2_SENSE */
	const bool usbConnected = true;
#endif /* PIN_USB2_SENSE */
	if (*buf <= RequestType::GET_KEYBOARD_LEDS || (usbConnected && USBDevice.configured())) {
		FrameParams fp(seq, static_cast<RequestType::Type>(*buf), buf + 1, size_t(len - 1));
		fn(fp);
	} else {
		/* Tried to send data to the host without a USB connection. */
#ifdef PIN_STATUS_LED
		setStatusLed(false);
#endif /* PIN_STATUS_LED */
		sendResponse(seq, ResponseType::E_HOST_WRITE_ERROR);
		if ( ! usbConnected ) {
			Serial1.flush();
			USBDevice.detach();
			USBDevice.attach();
		}
		return;
	}
}


/**
 * Writes out a single byte to the controller.
 *
 * @param[in] userArg - user defined callback argument
 * @param[in] val - byte to send
 * @param[in] eof - end-of-frame mark
 * @return true on success, else false
 */
inline bool handleWrite(void * /* userArg */, const uint8_t val, const bool eof) {
	const bool res = Serial1.write(val) == 1;
	if ( eof ) Serial1.flush();
	return res;
}


/**
 * Handles protocol version request.
 * Returns the protocol version as uint16_t.
 *
 * @param[in] fp - frame parameters
 */
void getProtocolVersion(const FrameParams & fp) {
	/* assume new connection and ensure proper start of frame */
	framing.setFirstOut();
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK, uint16_t(VKVM_PROT_VERSION));
}


/**
 * Handles alive request.
 *
 * @param[in] fp - frame parameters
 */
void getAlive(const FrameParams & fp) {
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


/**
 * Handles get USB state request.
 * Returns the USB state as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void getUsbState(const FrameParams & fp) {
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK, getCurrentUsbState());
}


/**
 * Handles get keyboard LED request.
 * Returns the keyboard LED states as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void getKeyboardLeds(const FrameParams & fp) {
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK, Vkm().getLeds());
}


/**
 * Handles set keyboard down request.
 * Expects up to 6 key codes each as uint8_t.
 * Returns a bit map with the keys actually pressed whereas the LSB is the first key.
 *
 * @param[in] fp - frame parameters
 */
void setKeyboardDown(const FrameParams & fp) {
	if (fp.len < 1 || fp.len > 6) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	/* return bit field with the result for each key */
	uint8_t res = 0;
	uint8_t i = 0;
	for (; i < fp.len; i++) {
		res = uint8_t((res >> 1) | (Vkm().pressKey(fp.buf[i]) ? 0x20 : 0x00));
	}
	for (; i < 6; i++) res = uint8_t(res >> 1);
	sendResponse(fp.seq, ResponseType::S_OK, res);
}


/**
 * Handles set keyboard up request.
 * Expects up to 6 key codes each as uint8_t.
 * Returns a bit map with the keys actually released whereas the LSB is the first key.
 *
 * @param[in] fp - frame parameters
 */
void setKeyboardUp(const FrameParams & fp) {
	if (fp.len < 1 || fp.len > 6) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	/* return bit field with the result for each key */
	uint8_t res = 0;
	uint8_t i = 0;
	for (; i < fp.len; i++) {
		res = uint8_t((res > 1) | (Vkm().releaseKey(fp.buf[i]) ? 0x20 : 0x00));
	}
	for (; i < 6; i++) res = uint8_t(res >> 1);
	sendResponse(fp.seq, ResponseType::S_OK, res);
}


/**
 * Handles set keyboard all up request.
 *
 * @param[in] fp - frame parameters
 */
void setKeyboardAllUp(const FrameParams & fp) {
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	if ( ! Vkm().releaseAllKeys() ) {
		sendResponse(fp.seq, ResponseType::E_HOST_WRITE_ERROR);
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


/**
 * Handles set keyboard push request.
 * Expects any number of ASCII characters each as uint8_t.
 * Returns the number of characters sent as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void setKeyboardPush(const FrameParams & fp) {
	const uint8_t res = uint8_t(Vkm().pushKeys(fp.buf, fp.len));
	sendResponse(fp.seq, ResponseType::S_OK, res);
}


/**
 * Handles set keyboard write request.
 * Expects one modifier as uint8_t and any number of ASCII characters each as uint8_t.
 * Returns the number of characters sent as uint8_t excluding the modifier.
 *
 * @param[in] fp - frame parameters
 */
void setKeyboardWrite(const FrameParams & fp) {
	if (fp.len < 1) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	const uint8_t res = uint8_t(Vkm().write(fp.buf[0], fp.buf + 1, fp.len - 1));
	sendResponse(fp.seq, ResponseType::S_OK, res);
}


/**
 * Handles set mouse button down request.
 * Expects up to 3 mouse buttons each as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void setMouseButtonDown(const FrameParams & fp) {
	if (fp.len < 1 || fp.len > 3) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	uint8_t brokenIndex = 0xFF;
	for (uint8_t i = 0; i < fp.len && i < 3; i++) {
		if ((fp.buf[i] & USBBUTTON_ALL) == fp.buf[i]) {
			Vkm().pressButton(fp.buf[i]);
		} else {
			brokenIndex = i;
			break;
		}
	}
	if (brokenIndex != 0xFF) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, brokenIndex);
	} else {
		sendResponse(fp.seq, ResponseType::S_OK);
	}
}


/**
 * Handles set mouse button up request.
 * Expects up to 3 mouse buttons each as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void setMouseButtonUp(const FrameParams & fp) {
	if (fp.len < 1 || fp.len > 3) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	uint8_t brokenIndex = 0xFF;
	for (uint8_t i = 0; i < fp.len && i < 3; i++) {
		if ((fp.buf[i] & USBBUTTON_ALL) == fp.buf[i]) {
			Vkm().releaseButton(fp.buf[i]);
		} else {
			brokenIndex = i;
			break;
		}
	}
	if (brokenIndex != 0xFF) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, brokenIndex);
	} else {
		sendResponse(fp.seq, ResponseType::S_OK);
	}
}


/**
 * Handles set mouse button all up request.
 *
 * @param[in] fp - frame parameters
 */
void setMouseButtonAllUp(const FrameParams & fp) {
	if (fp.len > 0) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(0));
		return;
	}
	if ( ! Vkm().releaseButton(USBBUTTON_ALL) ) {
		sendResponse(fp.seq, ResponseType::E_HOST_WRITE_ERROR);
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


/**
 * Handles set mouse button push request.
 * Expects any number of mouse buttons each as uint8_t.
 * Returns the number of buttons sent as uint8_t.
 *
 * @param[in] fp - frame parameters
 */
void setMouseButtonPush(const FrameParams & fp) {
	bool broken = false;
	uint8_t brokenIndex = 0;
	uint8_t res = 0;
	for (uint8_t i = 0; i < fp.len; i++) {
		if ((fp.buf[i] & USBBUTTON_ALL) == fp.buf[i]) {
			res = uint8_t(res + Vkm().pushButton(fp.buf[i]));
		} else {
			broken = true;
			brokenIndex = i;
			break;
		}
	}
	if ( broken ) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, brokenIndex);
	} else {
		sendResponse(fp.seq, ResponseType::S_OK, res);
	}
}


/**
 * Handles set mouse move relative request.
 * Expects the following fields:
 * - int16_t with the position on the x-axis (0 = 0%, 32767 = 100%)
 * - int16_t with the position on the y-axis (0 = 0%, 32767 = 100%)
 *
 * @param[in] fp - frame parameters
 */
void setMouseMoveAbs(const FrameParams & fp) {
	if (fp.len != 4) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(fp.len));
		return;
	}
	const int16_t xVal = int16_t((int16_t(fp.buf[0]) << 8) | int16_t(fp.buf[1]));
	const int16_t yVal = int16_t((int16_t(fp.buf[2]) << 8) | int16_t(fp.buf[3]));
	if ( ! Vkm().moveAbs(xVal, yVal) ) {
		sendResponse(fp.seq, ResponseType::E_HOST_WRITE_ERROR);
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


/**
 * Handles set mouse move relative request.
 * Expects the following fields:
 * - int8_t with the amount to move along the x-axis
 * - int8_t with the amount to move along the y-axis
 *
 * @param[in] fp - frame parameters
 */
void setMouseMoveRel(const FrameParams & fp) {
	if (fp.len != 2) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(fp.len));
		return;
	}
	const int8_t xVal = int8_t(fp.buf[0]);
	const int8_t yVal = int8_t(fp.buf[1]);
	if ( ! Vkm().moveRel(xVal, yVal) ) {
		sendResponse(fp.seq, ResponseType::E_HOST_WRITE_ERROR);
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


/**
 * Handles set mouse scroll request.
 * Expects the following fields:
 * - int8_t with the amount to move scroll wheel
 *
 * @param[in] fp - frame parameters
 */
void setMouseScroll(const FrameParams & fp) {
	if (fp.len != 1) {
		sendResponse(fp.seq, ResponseType::E_INVALID_FIELD_VALUE, uint8_t(fp.len));
		return;
	}
	const int8_t wheel = int8_t(fp.buf[0]);
	if ( ! Vkm().scroll(wheel) ) {
		sendResponse(fp.seq, ResponseType::E_HOST_WRITE_ERROR);
		return;
	}
	sendResponse(fp.seq, ResponseType::S_OK);
}


#if defined(PIN_STATUS_LED) && defined(VKVM_LED_PWM)
/** Input buffer for the DMA transfer which sets the GPIO port bits of the status LED. */
static uint16_t statusLedDmaInput[2] = {
	uint16_t(getInternalPin(PIN_STATUS_LED)), /* LED off */
	0, /* LED on */
};
/** STM32 HAL specific DMA handle. */
static DMA_HandleTypeDef hDmaStatusLed[1] = {0};
#endif /* PIN_STATUS_LED and VKVM_LED_PWM */


/**
 * Initializes the status LED, if available.
 */
void initStatusLed(void) {
#ifdef PIN_STATUS_LED
	pinMode(PIN_STATUS_LED, OUTPUT_OPEN_DRAIN);
	digitalWrite(PIN_STATUS_LED, LED_OFF);
#ifdef VKVM_LED_PWM
	/* Initialize TIM1 for PWM output. */
	const _TimerPinMap timInfo{1, 2, 1, 0}; /* TIM1_CH3N */
	TIM_TypeDef * inst = reinterpret_cast<TIM_TypeDef *>(timInfo.getInstanceBaseAddr());
	TIM_HandleTypeDef * hTim = _TimerPinMap::getTimHandleFromId(inst);
	if (hTim == NULL) systemErrorHandler();
	TIM_OC_InitTypeDef pwmChannel = {0};
	TIM_ClockConfigTypeDef clkSrc = {0};
	const uint32_t channel = timInfo.getChannel();
	hTim->Instance = inst;
	/* Rounds to the next higher possible number of periods per second. */
	hTim->Init.Prescaler = uint32_t(timInfo.getTimerClockFrequency() / (200 * 0xFFFFUL));
	/* The interval from 0xFFFE to 0 also counts as a single interval which is why there are 0xFFFF intervals here. */
	hTim->Init.Period = 0xFFFE;
	hTim->Init.CounterMode = TIM_COUNTERMODE_UP;
	hTim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	hTim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_PWM_Init(hTim) != HAL_OK) systemErrorHandler();
	clkSrc.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(hTim, &clkSrc) != HAL_OK) systemErrorHandler();
	pwmChannel.OCMode = TIM_OCMODE_PWM1;
	pwmChannel.Pulse = VKVM_LED_PWM;
	pwmChannel.OCPolarity = TIM_OCPOLARITY_HIGH;
	pwmChannel.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	pwmChannel.OCFastMode = TIM_OCFAST_DISABLE;
	pwmChannel.OCIdleState = TIM_OCIDLESTATE_RESET;
	pwmChannel.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	HAL_TIM_PWM_ConfigChannel(hTim, &pwmChannel, channel);
	/* Initialize DMA. */
	__HAL_RCC_DMA1_CLK_ENABLE();
	hDmaStatusLed->Instance = DMA1_Channel5; /* triggered by TIM3_CH3/TIM3_UP */
	hDmaStatusLed->Init.Direction = DMA_MEMORY_TO_PERIPH;
	hDmaStatusLed->Init.PeriphInc = DMA_PINC_ENABLE;
	hDmaStatusLed->Init.MemInc = DMA_MINC_ENABLE;
	hDmaStatusLed->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hDmaStatusLed->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hDmaStatusLed->Init.Mode = DMA_CIRCULAR;
	hDmaStatusLed->Init.Priority = DMA_PRIORITY_LOW;
	if (HAL_DMA_Init(hDmaStatusLed) != HAL_OK) systemErrorHandler();
	/* Start DMA transfer to toggle status LED according to TIM3 output. */
	if (HAL_DMA_Start(
		hDmaStatusLed,
		reinterpret_cast<uint32_t>(statusLedDmaInput),
		reinterpret_cast<uint32_t>(&(getInternalPort(digitalPinToPort(PIN_STATUS_LED))->BSRR)),
		2 /* x uint16_t */
	) != HAL_OK) systemErrorHandler();
	if (HAL_TIMEx_PWMN_Start(hTim, channel) != HAL_OK) systemErrorHandler();
	__HAL_TIM_ENABLE_DMA(hTim, TIM_DMA_UPDATE | TIM_DMA_CC3); /* LED on/off */
#endif /* VKVM_LED_PWM */
	setStatusLed(false);
#endif /* PIN_STATUS_LED */
}


/**
 * Sets the status LED state.
 * 
 * @param[in] on - true to enable the LED, else false
 */
void setStatusLed(const bool on) {
#ifdef PIN_STATUS_LED
#ifdef VKVM_LED_PWM
	const_cast<volatile uint16_t *>(statusLedDmaInput)[1] = on ? uint16_t(getInternalPin(PIN_STATUS_LED)) : 0;
#else /* not VKVM_LED_PWM */
	digitalWrite(PIN_STATUS_LED, on ? LED_ON : LED_OFF);
#endif /* not VKVM_LED_PWM */
#else /* not PIN_STATUS_LED */
	(void)on;
#endif /* not PIN_STATUS_LED */
}


/**
 * Initialize the environment.
 */
void setup(void) {
#ifdef PIN_USB2_SENSE
	pinMode(PIN_USB2_SENSE, INPUT_PULLDOWN);
#endif /* PIN_USB2_SENSE */
	initStatusLed();
	Serial1.begin(VKVM_PROT_SPEED);
	Vkm().begin();
}


/**
 * Main processing loop.
 */
void loop(void) {
	static uint8_t leds = 0;
	static uint8_t state = getCurrentUsbState();
#ifdef PIN_STATUS_LED
	static uint32_t statusLedFlush = millis();
	static bool lastStatusLedOn = false;
#endif /* PIN_STATUS_LED */
#if defined(DEBUG) || defined(PIN_STATUS_LED)
	const uint32_t now = millis();
#endif /* DEBUG or PIN_STATUS_LED */
#ifdef DEBUG
	static const char hexStr[] = "0123456789ABCDEF";
	static uint32_t lastPing = 0;
	if ((now - lastPing) > 1000) {
		DBG_MSG("alive");
		lastPing = now;
	}
#endif /* DEBUG */
	/* Process data from the controller. */
	if (Serial1.available() > 0) {
		const uint8_t val = uint8_t(Serial1.read());
#ifdef DEBUG
		const char dbgStr[] = {hexStr[val >> 4], hexStr[val & 0x0F], 0};
		DBG_MSG(dbgStr);
#endif /* DEBUG */
		if ( ! framing.read(val, handleFrame) ) {
			sendResponse(0, ResponseType::E_BROKEN_FRAME);
		}
	}
	/* Process LED changes. */
	const uint8_t curLeds = Vkm().getLeds();
	if (curLeds != leds) {
		sendResponse(0, ResponseType::I_LED_UPDATE, curLeds);
		leds = curLeds;
	}
	/* Process USB periphery state changes. */
	const uint8_t curState = getCurrentUsbState();
	if (curState != state) {
		sendResponse(0, ResponseType::I_USB_STATE_UPDATE, curState);
		state = curState;
	}
#ifdef PIN_STATUS_LED
	/* Update USB connection status LED. */
	if ((now - statusLedFlush) >= LED_FLUSH_TIME) {
		lastStatusLedOn = !lastStatusLedOn;
		statusLedFlush = now;
	}
	const bool statusLedFlushing = USBDevice.configured() || lastStatusLedOn;
#ifdef PIN_USB2_SENSE
	setStatusLed(digitalRead(PIN_USB2_SENSE) && statusLedFlushing);
#else /* not PIN_USB2_SENSE */
	setStatusLed(statusLedFlushing);
#endif /* PIN_USB2_SENSE */
#endif /* PIN_STATUS_LED */
}
