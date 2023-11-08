/**
 * @file Draw.cpp
 * @author Daniel Starke
 * @date 2019-12-21
 * @version 2019-12-21
 */
#include <stdexcept>
#include <pcf/image/Draw.hpp>


namespace pcf {
namespace image {


/**
 * Blends the foreground color over the opaque background color.
 *
 * @param[in] fg - foreground color
 * @param[in] bg - background color
 * @return blended color
 */
pcf::color::SplitColor blendOver(const pcf::color::SplitColor & fg, const pcf::color::SplitColor & bg) {
	const float ai = 1.0f - fg[3];
	return pcf::color::SplitColor(
		(fg[0] * ai) + (bg[0] * fg[3]),
		(fg[1] * ai) + (bg[1] * fg[3]),
		(fg[2] * ai) + (bg[2] * fg[3]),
		0.0f
	);
}


} /* namespace image */
} /* namespace pcf */
