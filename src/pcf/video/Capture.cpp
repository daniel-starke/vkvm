/**
 * @file Capture.cpp
 * @author Daniel Starke
 * @date 2019-10-03
 * @version 2026-06-14
 */
#include <cstddef>
#include <cstdint>
#include <libpcf/target.h>


namespace pcf {
namespace video {


/**
 * Returns the number of bytes per frame with 3 bytes per pixel.
 * Guards against size_t multiplication overflow.
 *
 * @param[in] width - frame width in pixels
 * @param[in] height - frame height in pixels
 * @param[out] bytes - receives width*height*3 on success (unchanged on failure)
 * @return true if the size is non-zero and fits within size_t, else false
 */
inline bool getFrameBytes(const size_t width, const size_t height, size_t & bytes) {
	if (width == 0 || height == 0) return false;
	if (width > (SIZE_MAX / 3)) return false;
	const size_t rowBytes = width * 3;
	if (height > (SIZE_MAX / rowBytes)) return false;
	bytes = rowBytes * height;
	return true;
}


} /* namespace video */
} /* namespace pcf */


#ifdef PCF_IS_WIN
#include "CaptureDirectShow.ipp"
#else
#ifdef PCF_IS_LINUX
#include "CaptureVideo4Linux2.ipp"
#else
#error Unsupported target OS.
#endif
#endif
