/**
 * @file Draw.hpp
 * @author Daniel Starke
 * @date 2019-12-21
 * @version 2019-12-22
 */
#ifndef __PCF_IMAGE_DRAW_HPP__
#define __PCF_IMAGE_DRAW_HPP__

#include <cmath>
#include <cstddef>
#include <pcf/color/SplitColor.hpp>


namespace pcf {
namespace image {


pcf::color::SplitColor blendOver(const pcf::color::SplitColor & fg, const pcf::color::SplitColor & bg);


/**
 * Draws an anti-aliased circle with the given parameters.
 *
 * @param[in] setFn - sets the RGBA value of a pixel
 * @param[in] x - center position on the x axis
 * @param[in] y - center position on the y axis
 * @param[in] r - radius (maximum extend)
 * @param[in] t - thickness (fully filled if r == t)
 * @param[in] color - RGBA color value for the circle
 * @tparam Fn - void Fn(const size_t x, const size_t y, const pcf::color::SplitColor & color)
 */
template <typename Fn>
static void drawCircleAA(Fn fn, const size_t x, const size_t y, const size_t r, const size_t t, const pcf::color::SplitColor & color) {
	auto set4 = [&, x, y](const size_t dx, const size_t dy, const pcf::color::SplitColor & c) {
		fn(x + dx, y + dy, c);
		fn(x - dx, y + dy, c);
		fn(x + dx, y - dy, c);
		fn(x - dx, y - dy, c);
	};
	const size_t rt = size_t(r - t);
	const float rf = float(r);
	const float rtf = float(rt);
	const float rr = rf * rf;
	const float rtrt = rtf * rtf;
	const pcf::color::SplitColor baseColor(color[0], color[1], color[2]);
	const size_t q = size_t((rr / sqrt(rr + rr)) + 0.5f);
	for (size_t xi = 0; xi <= q; xi++) {
		const float xixi = float(xi * xi);
		const float y1f = rf * sqrtf(1.0f - (xixi / rr));
		const float y2f = (xixi < rtrt) ? rtf * sqrtf(1.0f - (xixi / rtrt)) : 0.0f;
		const float err1 = y1f - truncf(y1f);
		const float err2 = y2f - truncf(y2f);
		const float alpha1 = (err1 <= 1.0f) ? 1.0f - err1 : 1.0f;
		const float alpha2 = (err2 <= 1.0f) ? err2 : 0.0f;
		const size_t y1i = size_t(y1f) + 1;
		const size_t y2i = size_t(y2f);
		/* outer circle edge case */
		set4(xi, y1i, pcf::color::SplitColor(baseColor, alpha1));
		set4(y1i, xi, pcf::color::SplitColor(baseColor, alpha1));
		/*  inner circle edge case */
		set4(xi, y2i, pcf::color::SplitColor(baseColor, alpha2));
		set4(y2i, xi, pcf::color::SplitColor(baseColor, alpha2));
		/* line from outer to inner circle edge */
		for (size_t yi = y2i + 1; yi < y1i; yi++) {
			set4(xi, yi, baseColor);
			set4(yi, xi, baseColor);
		}
	}
	/* center point */
	if (t >= r) fn(x, y, baseColor);
}


/**
 * Draws an anti-aliased ellipse with the given parameters.
 *
 * @param[in] setFn - sets the RGBA value of a pixel
 * @param[in] x - center position on the x axis
 * @param[in] y - center position on the y axis
 * @param[in] w - full width (maximum extend)
 * @param[in] h - full height (maximum extend)
 * @param[in] t - thickness (fully filled if min(w, h) == t)
 * @param[in] color - RGBA color value for the circle
 * @tparam Fn - void Fn(const size_t x, const size_t y, const pcf::color::SplitColor & color)
 */
template <typename Fn>
static void drawEllipseAA(Fn fn, const size_t x, const size_t y, const size_t w, const size_t h, const size_t t, const pcf::color::SplitColor & color) {
	if (w == h) {
		drawCircleAA(fn, x, y, w / 2, t, color);
		return;
	}
	auto set4 = [&, x, y](const size_t dx, const size_t dy, const pcf::color::SplitColor & c) {
		fn(x + dx, y + dy, c);
		fn(x - dx, y + dy, c);
		fn(x + dx, y - dy, c);
		fn(x - dx, y - dy, c);
	};
	const size_t rx = w / 2;
	const size_t ry = h / 2;
	const size_t rxt = size_t(rx - t);
	const size_t ryt = size_t(ry - t);
	const float rxtf = float(rxt);
	const float rytf = float(ryt);
	const float rxf = float(rx);
	const float ryf = float(ry);
	const float rxrx = rxf * rxf;
	const float ryry = ryf * ryf;
	const float rxtrxt = rxtf * rxtf;
	const float rytryt = rytf * rytf;
	const pcf::color::SplitColor baseColor(color[0], color[1], color[2]);
	/* filled upper and lower quarter */
	const size_t qx = size_t((rxrx / sqrtf(rxrx + ryry)) + 0.5f);
	for (size_t xi = 1; xi <= qx; xi++) {
		const float xixi = float(xi * xi);
		const float y1f = ryf * sqrtf(1.0f - (xixi / rxtrxt));
		const float y2f = (xixi < rxtrxt) ? rytf * sqrtf(1.0f - (xixi / rxtrxt)) : 0.0f;
		const float err1 = y1f - truncf(y1f);
		const float alpha1 = (err1 <= 1.0f) ? 1.0f - err1 : 1.0f;
		const size_t y1i = size_t(y1f) + 1;
		const size_t y2i = size_t(y2f);
		/* outer ellipse edge case */
		set4(xi, y1i, pcf::color::SplitColor(baseColor, alpha1));
		/* line from outer to inner ellipse edge */
		for (size_t yi = y2i + 1; yi < y1i; yi++) {
			set4(xi, yi, baseColor);
		}
	}
	const size_t qtx = size_t((rxtrxt / sqrtf(rxtrxt + rytryt)) + 0.5f);
	for (size_t xi = 1; xi <= qtx; xi++) {
		const float xixi = float(xi * xi);
		const float y2f = rytf * sqrtf(1.0f - (xixi / rxtrxt));
		const float err2 = y2f - truncf(y2f);
		const float alpha2 = (err2 <= 1.0f) ? err2 : 0.0f;
		/* inner ellipse edge case */
		set4(xi, size_t(y2f), pcf::color::SplitColor(baseColor, alpha2));
	}
	/* filled left and right quarter */
	const size_t qy = size_t((ryry / sqrtf(rxrx + ryry)) + 0.5f);
	for (size_t yi = 1; yi <= qy; yi++) {
		const float yiyi = float(yi * yi);
		const float x1f = rxf * sqrtf(1.0f - (yiyi / ryry));
		const float x2f = (yiyi < rytryt) ? rxtf * sqrtf(1.0f - (yiyi / rytryt)) : 0.0f;
		const float err1 = x1f - truncf(x1f);
		const float alpha1 = (err1 <= 1.0f) ? 1.0f - err1 : 1.0f;
		const size_t x1i = size_t(x1f) + 1;
		const size_t x2i = size_t(x2f);
		/* outer ellipse edge case */
		set4(x1i, yi, pcf::color::SplitColor(baseColor, alpha1));
		/* line from outer to inner ellipse edge */
		for (size_t xi = (x2i < qx) ? qx : x2i; xi < x1i; xi++) {
			set4(xi, yi, baseColor);
		}
	}
	const size_t qty = size_t((rytryt / sqrtf(rxtrxt + rytryt)) + 0.5f);
	for (size_t yi = 1; yi <= qty; yi++) {
		const float yiyi = float(yi * yi);
		const float x2f = rxtf * sqrtf(1.0f - (yiyi / rytryt));
		const float err2 = x2f - truncf(x2f);
		const float alpha2 = (err2 <= 1.0f) ? err2 : 0.0f;
		/* inner ellipse edge case */
		set4(size_t(x2f), yi, pcf::color::SplitColor(baseColor, alpha2));
	}
	/* center lines */
	for (size_t i = 0; i <= t; i++) {
		fn(x - rx + i, y, baseColor);
		fn(x + rx - i, y, baseColor);
		fn(x, y - ry + i, baseColor);
		fn(x, y + ry - i, baseColor);
	}
}


} /* namespace image */
} /* namespace pcf */


#endif /* __PCF_IMAGE_SVG_HPP__ */
