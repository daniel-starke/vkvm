/**
 * @file Framing.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2023 Daniel Starke
 * @date 2019-02-20
 * @version 2023-10-24
 */
#ifndef __FRAMING_HPP__
#define __FRAMING_HPP__

#include <stdint.h>
#include "Crc16.hpp"
#include "Meta.hpp"
#if defined(ARDUINO)
#include "Arduino.h"
#else
extern "C" unsigned long millis(void);
#endif


#if !defined(__BYTE_ORDER__) || !defined(__ORDER_BIG_ENDIAN__)
#error Target endianess was not specified.
#else /* __BYTE_ORDER__ */
#define __FRAMING_HPP__IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#endif /* __BYTE_ORDER__ */


/**
 * Simple implementation of a RFC 1662 like framing protocol. The data is protected with a CRC16 over the
 * unquoted payload. A frame is given in the following format with all values being encoded big endian:
 * <SEP>Quote(<Sequence><Payload><CRC16>)<SEP>
 *
 * @tparam MaxFrameSize - maximum size of the receiving unquoted payload data
 * @remarks The CRC check takes around 7us per byte on an ATmega32U4 at 16 MHz.
 */
template <size_t MaxFrameSize>
class Framing {
public:
	/**
	 * Write callback handler type.
	 *
	 * @param[in,out] user - user defined callback argument
	 * @param[in] val - byte value to wrie
	 * @param[in] eof - end of frame flag
	 * @return true if successfully written, else false
	 */
	typedef bool (* WriteCallback)(void * /* user */, const uint8_t /* val */, const bool /* eof */);
	enum {
		SEP = 0x7E, /**< frame separator */
		ESC = 0x7D, /**< escape byte */
		FLIP = 0x20 /**< bit mask to invert on escape */
	};
private:
	/** Possible states for the frame receiver. */
	enum State {
		ST_START, /**< searching for the start tag of a frame */
		ST_SEP, /**< searching for the end tag of a frame */
		ST_ESC /**< byte was escaped */
	};
	WriteCallback writer; /**< function called to send out a single byte */
	void * userArg; /**< user defined callback argument of the write handler */
	uint8_t buffer[MaxFrameSize + 3]; /**< receiving data parser buffer (incl. sequence number and CRC16) */
	size_t size; /**< used buffer size */
	State state; /**< current state of the parser */
	Crc16 crc; /**< intermediate CRC16 value of the send routine */
	bool firstOut; /**< true if the next frame sent is the first, else false */
	unsigned long lastOut; /**< value of `millis()` when the last frame was sent */
public:
	/**
	 * Constructor.
	 *
	 * @param[in] w - function called to send out a single byte
	 * @param[in,out] ua - user defined callback argument
	 */
	explicit Framing(WriteCallback w, void * ua = NULL):
		writer(w),
		userArg(ua),
		size(0),
		state(ST_START),
		firstOut(true)
	{}

	/**
	 * Processes a single byte received and calls the given function for each complete frame.
	 *
	 * @param[in] val - received byte value
	 * @param[in] fn - callback function as void fn(const uint8_t seq, uint8_t * buf, const size_t len, const bool err)
	 * @return true if successful, false on buffer overrun or underrun
	 */
	template <typename Fn>
	bool read(const uint8_t val, Fn fn) {
		retry:
		switch (this->state) {
		case ST_START:
			if (val == SEP) this->state = ST_SEP;
			break;
		case ST_SEP:
			switch (val) {
			case ESC:
				this->state = ST_ESC;
				break;
			case SEP:
				if (this->size == 0) {
					break; /* ignore empty frames */
				} else if (this->size < 3) {
					this->size = 0;
					return false; /* incomplete frame */
				} else {
					uint8_t * ptr = this->buffer + this->size - 2;
					const uint16_t containedCrc = uint16_t((uint16_t(ptr[0]) << 8) | uint16_t(ptr[1]));
					const uint16_t calculatedCrc = Crc16(this->buffer, ptr);
					fn(this->buffer[0], this->buffer + 1, size_t(this->size - 3), containedCrc != calculatedCrc);
					this->size = 0;
				}
				break;
			default:
				if (this->size >= MaxFrameSize) return false;
				this->buffer[this->size++] = val;
				break;
			}
			break;
		case ST_ESC:
			switch (val) {
			case ESC:
			case SEP:
				this->state = ST_SEP;
				goto retry;
			default:
				if (this->size >= MaxFrameSize) return false;
				this->buffer[this->size++] = uint8_t(val ^ FLIP);
				this->state = ST_SEP;
				break;
			}
			break;
		}
		return true;
	}

	/**
	 * Ensures that the next transmission sends the frame separator at the beginning.
	 */
	void setFirstOut() {
		this->firstOut = true;
	}

	/**
	 * Starts the transmission of a frame. Terminate each frame with a call to
	 * endTransmission().
	 *
	 * @param[in] seq - sequence number
	 * @return true on success, else false
	 */
	bool beginTransmission(const uint8_t seq = 0) {
		const unsigned long now = millis();
		this->crc = Crc16();
		if (this->firstOut || static_cast<unsigned long>(now - this->lastOut) > 1000) {
			if ( ! this->writer(this->userArg, uint8_t(SEP), false) ) return false;
		}
		this->firstOut = false;
		this->lastOut = now;
		return this->write(seq);
	}

	/**
	 * Ends the transmission of a frame.
	 *
	 * @return true on success, else false
	 */
	bool endTransmission() {
		const uint16_t finalCrc = uint16_t(this->crc);
		/* big-endian */
		if ( ! this->write(uint8_t((finalCrc >> 8) & 0xFF)) ) return false;
		if ( ! this->write(uint8_t(finalCrc & 0xFF)) ) return false;
		if ( ! this->writer(this->userArg, uint8_t(SEP), true) ) return false;
		return true;
	}

	/**
	 * Write a single payload byte. beginTransmission() needs to be called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	bool write(const uint8_t val) {
		this->crc(val);
		switch (val) {
		case SEP:
		case ESC:
			if ( ! this->writer(this->userArg, uint8_t(ESC), false) ) return false;
			if ( ! this->writer(this->userArg, uint8_t(val ^ FLIP), false) ) return false;
			break;
		default:
			if ( ! this->writer(this->userArg, val, false) ) return false;
			break;
		}
		return true;
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	template <typename T>
	typename enable_if<is_arithmetic<T>::value, bool>::type write(const T val) {
#if __FRAMING_HPP__IS_BIG_ENDIAN
		return this->write(reinterpret_cast<const uint8_t *>(&val), sizeof(T));
#else /* reverse little endian to big endian */
		const uint8_t * ptr = reinterpret_cast<const uint8_t *>(&val) - 1;
		for (size_t i = 0; i < sizeof(T); i++, ptr--) {
			if ( ! this->write(*ptr) ) return false;
		}
		return true;
#endif
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	bool write(const uint16_t val) {
		if ( ! this->write(uint8_t((val >> 8) & 0xFF)) ) return false;
		if ( ! this->write(uint8_t(val & 0xFF)) ) return false;
		return true;
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	bool write(const uint32_t val) {
		if ( ! this->write(uint8_t((val >> 24) & 0xFF)) ) return false;
		if ( ! this->write(uint8_t((val >> 16) & 0xFF)) ) return false;
		if ( ! this->write(uint8_t((val >> 8) & 0xFF)) ) return false;
		if ( ! this->write(uint8_t(val & 0xFF)) ) return false;
		return true;
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	inline bool write(const int8_t val) {
		return this->write(uint8_t(val));
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	inline bool write(const int16_t val) {
		return this->write(uint16_t(val));
	}

	/**
	 * Write a given value big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] val - payload byte value
	 * @return true on success, else false
	 */
	inline bool write(const int32_t val) {
		return this->write(uint32_t(val));
	}

	/**
	 * Helper function for parameter pack expansion. Writes nothing.
	 *
	 * @return true
	 */
	constexpr bool write() const {
		return true;
	}

	/**
	 * Writes the given values big endian encoded as payload data. beginTransmission() needs to be
	 * called beforehand.
	 *
	 * @param[in] args - values to write
	 * @return true on success, else false
	 */
	template <typename T0, typename ...Args>
	inline typename enable_if<is_arithmetic<T0>::value, bool>::type write(const T0 val, Args... args) {
		return this->write(val) && this->write(args...);
	}

	/**
	 * Write the given data as payload data. beginTransmission() needs to be called beforehand.
	 *
	 * @param[in] buf - data buffer
	 * @param[in] len - data size
	 * @return true on success, else false
	 */
	bool write(const uint8_t * buf, const size_t len) {
		for (size_t i = 0; i < len; i++) {
			if ( ! this->write(buf[i]) ) return false;
		}
		return true;
	}

	/**
	 * Constructs a frame from the given array and sends it out using the given function.
	 *
	 * @param[in] buf - frame to send
	 * @param[in] len - frame length
	 */
	bool send(const uint8_t * buf, const size_t len) {
		if ( ! this->beginTransmission() ) return false;
		if ( ! this->write(buf, len) ) return false;
		return this->endTransmission();
	}
};


#endif /* __FRAMING_HPP__ */
