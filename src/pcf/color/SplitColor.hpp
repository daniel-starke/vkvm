/**
 * @file SplitColor.hpp
 * @author Daniel Starke
 * @date 2017-05-10
 * @version 2023-10-03
 */
#ifndef __PCF_COLOR_SPLITCOLOR_HPP__
#define __PCF_COLOR_SPLITCOLOR_HPP__

#include <FL/Fl.H>


namespace pcf {
namespace color {


/**
 * Helper to hold and manage RGB / HSV values.
 * All values are in the range from 0 to 1.
 */
struct SplitColor {
	float value[4];

	explicit inline SplitColor() {
		this->value[0] = 0.0f;
		this->value[1] = 0.0f;
		this->value[2] = 0.0f;
		this->value[3] = 0.0f;
	}

	template <typename T>
	explicit inline SplitColor(const T v1, const T v2 = T(0), const T v3 = T(0), const T v4 = T(0)) {
		this->value[0] = from(v1);
		this->value[1] = from(v2);
		this->value[2] = from(v3);
		this->value[3] = from(v4);
	}

	explicit inline SplitColor(const Fl_Color val) {
		uchar r, g, b;
		Fl::get_color(val, r, g, b);
		this->value[0] = from(r);
		this->value[1] = from(g);
		this->value[2] = from(b);
		this->value[3] = 0.0f;
	}

	template <typename T>
	explicit inline SplitColor(const Fl_Color val, const T v4) {
		uchar r, g, b;
		Fl::get_color(val, r, g, b);
		this->value[0] = from(r);
		this->value[1] = from(g);
		this->value[2] = from(b);
		this->value[3] = from(v4);
	}

	inline SplitColor(const SplitColor & val) {
		this->value[0] = val.value[0];
		this->value[1] = val.value[1];
		this->value[2] = val.value[2];
		this->value[3] = val.value[3];
	}

	inline SplitColor & operator =(const SplitColor & val) {
		if (this != &val) {
			this->value[0] = val.value[0];
			this->value[1] = val.value[1];
			this->value[2] = val.value[2];
			this->value[3] = val.value[3];
		}
		return *this;
	}

	inline float & operator [](const size_t idx) { return this->value[idx]; }
	inline float operator [](const size_t idx) const { return this->value[idx]; }

	template <typename T>
	inline T get(const size_t idx) const { return to<T>(this->value[idx]); }

	template <typename T>
	inline void set(const size_t idx, const T val) { this->value[idx] = from(val); }

	inline operator Fl_Color() const {
		int part[3];
		for (size_t k = 0; k < 3; k++) {
			const int v = int((this->value[k] * 255.0f) + 0.5f);
			part[k] = v < 0 ? 0 : v > 255 ? 255 : v;
		}
		return Fl_Color((part[0] << 24) | (part[1] << 16) | (part[2] << 8));
	}

	SplitColor rgbToHsv() const;
	SplitColor hsvToRgb() const;

private:
	template <typename T>
	static inline float from(const T val) { return float(val) / 255.0f; }
	template <typename T>
	static inline T to(const float val) { return T((val * 255.0f) + 0.5f); }
};


template <>
inline float SplitColor::from(const float val) { return val; }
template <>
inline float SplitColor::from(const double val) { return float(val); }


template <>
inline float SplitColor::to(const float val) { return val; }
template <>
inline double SplitColor::to(const float val) { return double(val); }


} /* namespace color */
} /* namespace pcf */


#endif /* __PCF_COLOR_SPLITCOLOR_HPP__ */
