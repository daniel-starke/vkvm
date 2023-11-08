/**
 * @file arduino.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-10-11
 * @version 2023-11-06
 */
#ifndef __ARDUINO_HPP__
#define __ARDUINO_HPP__

#include <stdint.h>
#include "Debug.hpp"
#include "Framing.hpp"
#include "Protocol.hpp"
#include "Vkm.hpp"


#ifdef __AVR__
#include <avr/pgmspace.h>
#define VKVM_ROM PROGMEM
#define VKVM_ROM_READ_U8(x, o) pgm_read_byte((x) + (o))
#define VKVM_ROM_READ_U16(x, o) pgm_read_word((x) + (o))
#define VKVM_ROM_READ_U32(x, o) pgm_read_dword((x) + (o))
#define VKVM_ROM_READ_PTR(x, o) pgm_read_ptr((x) + (o))
#else
#define VKVM_ROM
#define VKVM_ROM_READ_U8(x, o) (x)[o]
#define VKVM_ROM_READ_U16(x, o) (x)[o]
#define VKVM_ROM_READ_U32(x, o) (x)[o]
#define VKVM_ROM_READ_PTR(x, o) (x)[o]
#endif /* __AVR__ */


#ifndef __AVR__
/** Pin for the status LED. */
#define PIN_STATUS_LED PF_1

/** Pin for USB2 VBUS sense. */
#define PIN_USB2_SENSE PA_4
#endif /* not __AVR__ */


/** Pin state to turn the LED off. */
#define LED_OFF HIGH
/** Pin state to turn the LED on. */
#define LED_ON LOW
/** LED flushing interval in milliseconds. */
#define LED_FLUSH_TIME 500 /* ms */


/** Received frame parameters. */
struct FrameParams {
	uint8_t seq; /**< Sequence number. */
	RequestType::Type req; /**< Request type. */
	const uint8_t * buf; /**< Data buffer. */
	size_t len; /**< Data buffer size. */

	/**
	 * Constructor.
	 *
	 * @param[in] s - sequence number of the received frame
	 * @param[in] o - request type of the received frame
	 * @param[in] b - buffer to the request parameters
	 * @param[in] l - length of the buffer
	 */
	explicit FrameParams(const uint8_t s, const RequestType::Type r, const uint8_t * b, const size_t l):
		seq(s),
		req(r),
		buf(b),
		len(l)
	{}
};


/** Defines the function type for the request type handlers. */
typedef void (* tHandler)(const FrameParams & fp);


/** Framing protocol handler. */
extern Framing<VKVM_MAX_FRAME_SIZE> framing;


/** Request type handler array. */
extern const tHandler handler[RequestType::COUNT] VKVM_ROM;


/**
 * Returns an error to the host.
 *
 * @param[in] seq - sequence number
 * @param[in] type - result type
 * @tparam ...Args - argument types
 */
template <typename ...Args>
void sendResponse(const uint8_t seq, const ResponseType::Type type, Args... args) {
	framing.beginTransmission(seq);
	framing.write(uint8_t(type));
	framing.write(args...);
	framing.endTransmission();
}


void handleFrame(const uint8_t seq, const uint8_t * buf, const size_t len, const bool err);
inline bool handleWrite(void * userArg, const uint8_t val, const bool eof);
void getProtocolVersion(const FrameParams & fp);
void getAlive(const FrameParams & fp);
void getUsbState(const FrameParams & fp);
void getKeyboardLeds(const FrameParams & fp);
void setKeyboardDown(const FrameParams & fp);
void setKeyboardAllUp(const FrameParams & fp);
void setKeyboardUp(const FrameParams & fp);
void setKeyboardPush(const FrameParams & fp);
void setKeyboardWrite(const FrameParams & fp);
void setMouseButtonDown(const FrameParams & fp);
void setMouseButtonUp(const FrameParams & fp);
void setMouseButtonAllUp(const FrameParams & fp);
void setMouseButtonPush(const FrameParams & fp);
void setMouseMoveAbs(const FrameParams & fp);
void setMouseMoveRel(const FrameParams & fp);
void setMouseScroll(const FrameParams & fp);
void initStatusLed(void);
void setStatusLed(const bool on);
void setup(void);
void loop(void);


#endif /* __ARDUINO_HPP__ */
