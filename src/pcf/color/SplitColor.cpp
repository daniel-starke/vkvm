/**
 * @file SplitColor.cpp
 * @author Daniel Starke
 * @date 2017-05-10
 * @version 2019-10-06
 */
#include <pcf/color/SplitColor.hpp>


namespace pcf {
namespace color {


/**
 * Converts the HSV color values to RGB.
 *
 * @return RGB color values
 * @remarks All values are in the range from 0 to 1.
 */
SplitColor SplitColor::rgbToHsv() const {
	enum Index { R, G, B };
	float min = this->value[0];
	float max = this->value[0];
	Index maxValue = R;
	if (this->value[1] > max) { max = this->value[1]; maxValue = G; }
	if (this->value[2] > max) { max = this->value[2]; maxValue = B; }
	if (this->value[1] < min) min = this->value[1];
	if (this->value[2] < min) min = this->value[2];
	const float delta = max - min;
	const float v = max;
	if (max > 0.0f && delta > 0.000001f) {
		const float s = delta / max;
		float h = 0.0f;
		switch (maxValue) {
		case R: h = (        (this->value[1] - this->value[2]) / delta ) / 6.0f; if (h < 0.0f) h += 1.0f; break;
		case G: h = (2.0f + ((this->value[2] - this->value[0]) / delta)) / 6.0f; break;
		case B: h = (4.0f + ((this->value[0] - this->value[1]) / delta)) / 6.0f; break;
		}
		return SplitColor(h, s, v, this->value[3]);
	}
	return SplitColor(0.0f, 0.0f, v, this->value[3]);
}


/**
 * Converts the RGB color values to HSV.
 *
 * @return HSV color values
 * @remarks All values are in the range from 0 to 1.
 */
SplitColor SplitColor::hsvToRgb() const {
	const float h = 6.0f * this->value[0];
	const float s = this->value[1];
	const float v = this->value[2];
	if (s < 0.000001f) {
		return SplitColor(v, v, v, this->value[3]);
	} else {
		const float f = h - float(int(h));
		const float p = v * (1.0f - s);
		const float q = v * (1.0f - (s * f));
		const float t = v * (1.0f - (s * (1.0f - f)));
		switch (int(h) % 6) {
		case 0: return SplitColor(v, t, p, this->value[3]); break;
		case 1: return SplitColor(q, v, p, this->value[3]); break;
		case 2: return SplitColor(p, v, t, this->value[3]); break;
		case 3: return SplitColor(p, q, v, this->value[3]); break;
		case 4: return SplitColor(t, p, v, this->value[3]); break;
		case 5: return SplitColor(v, p, q, this->value[3]); break;
		default: return SplitColor(); break;
		}
	}
}


} /* namespace color */
} /* namespace pcf */
