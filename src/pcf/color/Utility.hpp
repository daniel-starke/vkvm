/**
 * @file Utility.hpp
 * @author Daniel Starke
 * @date 2019-10-01
 * @version 2023-10-03
 */
#ifndef __PCF_COLOR_UTILITY_HPP__
#define __PCF_COLOR_UTILITY_HPP__

#include <cstddef>
#include <cstdint>


namespace pcf {
namespace color {


/**
 * Enumeration of supported color format within the ColorFormat class namespace.
 */
struct ColorFormat {
	enum Type {
		RGB_555, /**< 16 bits per pixel, 5 bits per color, one bit unused, red/green/blue */
		RGB_565, /**< 16 bits per pixel, 5 bits red/blue, 6 bits green, red/green/blue */
		RGB_24, /**< 24 bits per pixel, 8 bits per color, red/green/blue */
		BGR_24, /**< 24 bits per pixel, 8 bits per color, blue/green/red */
		RGB_32, /**< 32 bits per pixel, 8 bits per color, red/green/blue/alpha */
		BGR_32, /**< 32 bits per pixel, 8 bits per color, blue/green/red/alpha */
		UNKNOWN /**< unknown color format */
	};
};


/**
 * Helper class to handle the RBG555 color format.
 */
struct Rgb555 {
	uint16_t value;

	explicit inline Rgb555():
		value(0)
	{}

	explicit inline Rgb555(const uint8_t r, const uint8_t g, const uint8_t b):
		value(uint16_t((uint16_t(r >> 3) << 10) | (uint16_t(g >> 3) << 5) | uint16_t(b >> 3)))
	{}

	inline uint8_t red() const {
		return uint8_t((this->value >> 7) & 0xF8);
	}

	inline void red(const uint8_t val) {
		this->value = uint16_t((this->value & 0x03FF) | (uint16_t(val >> 3) << 10));
	}

	inline uint8_t green() const {
		return uint8_t((this->value >> 2) & 0xF8);
	}

	inline void green(const uint8_t val) {
		this->value = uint16_t((this->value & 0x7C1F) | (uint16_t(val >> 3) << 5));
	}

	inline uint8_t blue() const {
		return uint8_t((this->value << 3) & 0xF8);
	}

	inline void blue(const uint8_t val) {
		this->value = uint16_t((this->value & 0x7FE0) | uint16_t(val >> 3));
	}
};


/**
 * Helper class to handle the RBG565 color format.
 */
struct Rgb565 {
	uint16_t value;

	explicit inline Rgb565():
		value(0)
	{}

	explicit inline Rgb565(const uint8_t r, const uint8_t g, const uint8_t b):
		value(uint16_t((uint16_t(r >> 3) << 11) | (uint16_t(g >> 2) << 5) | uint16_t(b >> 3)))
	{}

	inline uint8_t red() const {
		return uint8_t((this->value >> 8) & 0xF8);
	}

	inline void red(const uint8_t val) {
		this->value = uint16_t((this->value & 0x07FF) | (uint16_t(val >> 3) << 11));
	}

	inline uint8_t green() const {
		return uint8_t((this->value >> 3) & 0xFC);
	}

	inline void green(const uint8_t val) {
		this->value = uint16_t((this->value & 0xF81F) | (uint16_t(val >> 2) << 5));
	}

	inline uint8_t blue() const {
		return uint8_t((this->value << 3) & 0xF8);
	}

	inline void blue(const uint8_t val) {
		this->value = uint16_t((this->value & 0xFFE0) | uint16_t(val >> 3));
	}
};


/**
 * Helper class to handle the RBG24 color format.
 */
struct Rgb24 {
	uint8_t r;
	uint8_t g;
	uint8_t b;

	explicit inline Rgb24():
		r(0),
		g(0),
		b(0)
	{}

	explicit inline Rgb24(const uint8_t rVal, const uint8_t gVal, const uint8_t bVal):
		r(rVal),
		g(gVal),
		b(bVal)
	{}

	inline uint8_t red() const {
		return this->r;
	}

	inline void red(const uint8_t val) {
		this->r = val;
	}

	inline uint8_t green() const {
		return this->g;
	}

	inline void green(const uint8_t val) {
		this->g = val;
	}

	inline uint8_t blue() const {
		return this->b;
	}

	inline void blue(const uint8_t val) {
		this->b = val;
	}
};


/**
 * Helper class to handle the BGR24 color format.
 */
struct Bgr24 {
	uint8_t b;
	uint8_t g;
	uint8_t r;

	explicit inline Bgr24():
		b(0),
		g(0),
		r(0)
	{}

	explicit inline Bgr24(const uint8_t rVal, const uint8_t gVal, const uint8_t bVal):
		b(bVal),
		g(gVal),
		r(rVal)
	{}

	inline uint8_t red() const {
		return this->r;
	}

	inline void red(const uint8_t val) {
		this->r = val;
	}

	inline uint8_t green() const {
		return this->g;
	}

	inline void green(const uint8_t val) {
		this->g = val;
	}

	inline uint8_t blue() const {
		return this->b;
	}

	inline void blue(const uint8_t val) {
		this->b = val;
	}
};


/**
 * Helper class to handle the RBG32 color format.
 */
struct Rgb32 {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	explicit inline Rgb32():
		r(0),
		g(0),
		b(0),
		a(0)
	{}

	explicit inline Rgb32(const uint8_t rVal, const uint8_t gVal, const uint8_t bVal, const uint8_t aVal = 0):
		r(rVal),
		g(gVal),
		b(bVal),
		a(aVal)
	{}

	inline uint8_t red() const {
		return this->r;
	}

	inline void red(const uint8_t val) {
		this->r = val;
	}

	inline uint8_t green() const {
		return this->g;
	}

	inline void green(const uint8_t val) {
		this->g = val;
	}

	inline uint8_t blue() const {
		return this->b;
	}

	inline void blue(const uint8_t val) {
		this->b = val;
	}

	inline uint8_t alpha() const {
		return this->a;
	}

	inline void alpha(const uint8_t val) {
		this->a = val;
	}
};


/**
 * Helper class to handle the BGR32 color format.
 */
struct Bgr32 {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;

	explicit inline Bgr32():
		b(0),
		g(0),
		r(0),
		a(0)
	{}

	explicit inline Bgr32(const uint8_t rVal, const uint8_t gVal, const uint8_t bVal, const uint8_t aVal = 0):
		b(bVal),
		g(gVal),
		r(rVal),
		a(aVal)
	{}

	inline uint8_t red() const {
		return this->r;
	}

	inline void red(const uint8_t val) {
		this->r = val;
	}

	inline uint8_t green() const {
		return this->g;
	}

	inline void green(const uint8_t val) {
		this->g = val;
	}

	inline uint8_t blue() const {
		return this->b;
	}

	inline void blue(const uint8_t val) {
		this->b = val;
	}

	inline uint8_t alpha() const {
		return this->a;
	}

	inline void alpha(const uint8_t val) {
		this->a = val;
	}
};


} /* namespace color */
} /* namespace pcf */


#endif /* __PCF_COLOR_UTILITY_HPP__ */
