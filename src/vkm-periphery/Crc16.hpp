/**
 * @file Crc16.hpp
 * @author Daniel Starke
 * @copyright Copyright 2019-2020 Daniel Starke
 * @date 2019-03-07
 * @version 2020-05-31
 *
 * CRC16-CCITT used in HDLC (see RFC 1662).
 */
#ifndef __CRC16_HPP__
#define __CRC16_HPP__

#include <stdint.h>


#ifdef __AVR__
#include <avr/pgmspace.h>
#define __CRC16_HPP__ROM PROGMEM
#define __CRC16_HPP__ROM_READ_U16(x, o) pgm_read_word((x) + (o))
#else /* __AVR__ */
#define __CRC16_HPP__ROM
#define __CRC16_HPP__ROM_READ_U16(x, o) (x)[o]
#endif /* !__AVR__ */


/**
 * Table for fast CRC16 calculation (32 bytes).
 */
static const uint16_t crc16Table[16] __CRC16_HPP__ROM = {
    UINT16_C(0x0000), UINT16_C(0x1081), UINT16_C(0x2102), UINT16_C(0x3183),
	UINT16_C(0x4204), UINT16_C(0x5285), UINT16_C(0x6306), UINT16_C(0x7387),
    UINT16_C(0x8408), UINT16_C(0x9489), UINT16_C(0xA50A), UINT16_C(0xB58B),
	UINT16_C(0xC60C), UINT16_C(0xD68D), UINT16_C(0xE70E), UINT16_C(0xF78F)
};


/**
 * CRC16 class. Used for easy value initialization and finalization.
 */
class Crc16 {
private:
	uint16_t val;
public:
	/**
	 * Constructor.
	 */
	explicit Crc16():
		val(UINT16_C(0xFFFF))
	{}

	/**
	 * Constructor which calculates the CRC16 over the given byte.
	 *
	 * @param[in] value - byte value
	 */
	explicit Crc16(const uint8_t value):
		val(UINT16_C(0xFFFF))
	{
		this->operator() (value);
	}

	/**
	 * Constructor which calculates the CRC16 over the given iterator range.
	 *
	 * @param[in] first - beginning of the range
	 * @param[in] last - one past the end of the range
	 */
	template <typename It>
	explicit Crc16(It first, const It last):
		val(UINT16_C(0xFFFF))
	{
		this->operator() (first, last);
	}

	/**
	 * Calculates the CRC16 over the given byte.
	 *
	 * @param[in] value - byte value
	 * @return calculated CRC16
	 */
	Crc16 & operator() (const uint8_t value) {
		this->val = uint16_t(__CRC16_HPP__ROM_READ_U16(crc16Table, (this->val ^  value      ) & 0x0F) ^ (this->val >> 4));
		this->val = uint16_t(__CRC16_HPP__ROM_READ_U16(crc16Table, (this->val ^ (value >> 4)) & 0x0F) ^ (this->val >> 4));
		return *this;
	}

	/**
	 * Calculates the CRC16 over the given iterator range.
	 *
	 * @param[in] first - beginning of the range
	 * @param[in] last - one past the end of the range
	 * @return calculated CRC16
	 * @tparam It - iterator type
	 */
	template <typename It>
	Crc16 & operator() (It first, const It last) {
		while (first != last) {
			this->operator() (uint8_t(*first));
			++first;
		}
		return *this;
	}

	/**
	 * Cast to actually calculated value.
	 *
	 * @return final CRC16 value
	 */
	operator uint16_t() const {
		return uint16_t(~(this->val));
	}
};


#endif /* __CRC16_HPP__ */
