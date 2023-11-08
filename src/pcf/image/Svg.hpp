/**
 * @file Svg.hpp
 * @author Daniel Starke
 * @date 2017-04-10
 * @version 2023-06-06
 */
#ifndef __PCF_IMAGE_SVG_HPP__
#define __PCF_IMAGE_SVG_HPP__

#include <cstddef>
extern "C" {
#include <stdio.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif /* __clang__ */
#include <extern/nanosvg.h>
#include <extern/nanosvgrast.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif /* __clang__ */
} /* C */


namespace pcf {
namespace image {


/**
 * Renderer which uses the given SVG data to render an image as RGBA32 byte array.
 */
class SvgRenderer {
private:
	NSVGimage * svg;
	NSVGrasterizer * rast;
	unsigned char * buffer;
	size_t width;
	size_t height;
	bool didRender;
public:
	SvgRenderer(const char * aSvg = NULL);
	~SvgRenderer();

	void data(const char * aSvg);
	unsigned char * render(const size_t aWidth, const size_t aHeight, const bool force = false);
	bool redrawn() const { return this->didRender; }
};


} /* namespace image */
} /* namespace pcf */


#endif /* __PCF_IMAGE_SVG_HPP__ */
