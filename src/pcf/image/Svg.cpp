/**
 * @file Svg.cpp
 * @author Daniel Starke
 * @date 2017-04-10
 * @version 2019-10-06
 * @see https://www.w3.org/TR/SVG/Overview.html
 */
#include <pcf/image/Svg.hpp>
#include <FL/fl_ask.H>


#include <stdexcept>
extern "C" {
#include <stdio.h>
#include <string.h>
#include <float.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#endif /* __clang__ */
#define NANOSVG_IMPLEMENTATION
#include <extern/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <extern/nanosvgrast.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif /* __clang__ */
} /* C */


namespace pcf {
namespace image {


/**
 * Constructor.
 * Creates a new SvgRenderer instance from the given SVG string.
 *
 * @param[in] aSvg - SVG to render
 */
SvgRenderer::SvgRenderer(const char * aSvg):
	svg(NULL),
	rast(NULL),
	buffer(NULL),
	width(0),
	height(0)
{
	this->data(aSvg);
	this->rast = nsvgCreateRasterizer();
}


/**
 * Destructor.
 */
SvgRenderer::~SvgRenderer() {
	if (this->rast != NULL) nsvgDeleteRasterizer(this->rast);
	if (this->svg != NULL) nsvgDelete(this->svg);
	if (this->buffer != NULL) free(this->buffer);
}


/**
 * Changed the assigned SVG data.
 *
 * @param[in] aSvg - SVG to render
 */
void SvgRenderer::data(const char * aSvg) {
	if (aSvg == NULL) return;
	char * str = static_cast<char *>(strdup(aSvg));
	if (str == NULL) throw std::bad_alloc();
	if (this->svg != NULL) nsvgDelete(this->svg);
	this->svg = nsvgParse(str, "px", 96.0f);
	free(str);
	/* force re-rendering */
	this->width = 0;
	this->height = 0;
}


/**
 * Renders the stored SVG with the given dimensions.
 *
 * @param[in] aWidth - target image width
 * @param[in] aHeight - target image height
 * @param[in] force - force rendering even if nothing changed
 * @return rendered image as RGBA byte array with aWidth * aHeight * 4 bytes
 */
unsigned char * SvgRenderer::render(const size_t aWidth, const size_t aHeight, const bool force) {
	this->didRender = false;
	if (aWidth == 0 || aHeight == 0) {
		throw std::invalid_argument("SvgRenderer: invalid dimensions");
		return NULL;
	}
	if (this->buffer != NULL && this->width == aWidth && this->height == aHeight && !force) return this->buffer;
	if (this->buffer == NULL || (this->width * this->height) != (aWidth * aHeight)) {
		if (this->buffer != NULL) free(this->buffer);
		this->buffer = static_cast<unsigned char *>(malloc(aWidth * aHeight * 4));
		if (this->buffer == NULL) {
			throw std::bad_alloc();
			return NULL;
		}
	}
	float scale = float(aWidth) / float(svg->width);
	if (scale * float(svg->height) > float(aHeight)) {
		scale = float(aHeight) / float(svg->height);
	}
	nsvgRasterize(this->rast, this->svg, 0.0f, 0.0f, scale, this->buffer, int(aWidth), int(aHeight), int(aWidth * 4));
	this->didRender = true;
	this->width = aWidth;
	this->height = aHeight;
	return this->buffer;
}


} /* namespace image */
} /* namespace pcf */
