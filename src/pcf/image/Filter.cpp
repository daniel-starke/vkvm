/**
 * @file Filter.cpp
 * @author Daniel Starke
 * @date 2017-11-09
 * @version 2023-10-03
 */
#include <stdexcept>
#include <pcf/color/Utility.hpp>
#include <pcf/image/Filter.hpp>


/** Defines the used image buffer alignment size. */
#define IMAGE_ALIGNMENT 16


namespace pcf {
namespace image {


/**
 * Allocates an aligned image buffer.
 *
 * @param[in] pixels - number of pixels to allocate
 * @return aligned image buffer pointer or NULL on error
 * @remarks Free returned buffer via delImage().
 */
static inline pcf::color::Rgb32 * newImage(const size_t pixels) {
	unsigned char * rawPtr = static_cast<unsigned char *>(malloc((sizeof(pcf::color::Rgb32) * pixels) + IMAGE_ALIGNMENT));
	if (rawPtr == NULL) return NULL;
	unsigned char * alignedPtr = reinterpret_cast<unsigned char *>((uintptr_t(rawPtr) + IMAGE_ALIGNMENT) & uintptr_t(-IMAGE_ALIGNMENT));
	alignedPtr[-1] = static_cast<unsigned char>(alignedPtr - rawPtr);
	return reinterpret_cast<pcf::color::Rgb32 *>(alignedPtr);
}


/**
 * Frees the given aligned image buffer.
 *
 * @param[in] ptr - pointer returned by newImage()
 * @remarks ptr can be NULL
 */
static inline void delImage(pcf::color::Rgb32 * ptr) {
	if (ptr == NULL) return;
	unsigned char * alignedPtr = reinterpret_cast<unsigned char *>(ptr);
	free(alignedPtr - alignedPtr[-1]);
}


/**
 * PIMPLE implementation of the class attributes of pcf::image::Filter.
 */
struct FilterData {
	pcf::color::Rgb32 * image;
	size_t width;
	size_t height;

	/** Constructor. */
	explicit inline FilterData():
		image(NULL),
		width(0),
		height(0)
	{}

	/** Destructor. */
	inline ~FilterData() {
		delImage(this->image);
	}
};


/**
 * Constructor.
 */
Filter::Filter() : self(NULL) {
	self = new FilterData();
}


/**
 * Copy constructor.
 *
 * @param[in] o - copy this object
 */
Filter::Filter(const Filter & o) : self(NULL) {
	if (o.self != NULL) self = new FilterData(*(o.self));
}


/**
 * Destructor.
 */
Filter::~Filter() {
	if (self != NULL) delete self;
}


/**
 * Copy assignment constructor.
 *
 * @param[in] o - copy this object
 */
Filter & Filter::operator= (const Filter & o) {
	if (this == &o) return *this;
	if (self != NULL) delete self;
	if (o.self != NULL) self = new FilterData(*(o.self));
	else self = NULL;
	return *this;
}


/**
 * Returns the current image width.
 *
 * @return current image width
 */
size_t Filter::width() const {
	return self->width;
}


/**
 * Returns the current image height.
 *
 * @return current image height
 */
size_t Filter::height() const {
	return self->height;
}


/**
 * Frees the stored image.
 *
 * @return own instance reference for chained operation
 */
Filter & Filter::clear() {
	delImage(self->image);
	self->image = NULL;
	self->width = 0;
	self->height = 0;
	return *this;
}


/**
 * Loads the given image.
 *
 * @param[in] image - source image buffer
 * @param[in] width - source image width
 * @param[in] height - source image height
 * @param[in] format - source image format
 * @return own instance reference for chained operation
 */
Filter & Filter::load(const unsigned char * image, const size_t aWidth, const size_t aHeight, const ImageFormat format) {
	if (image == NULL) {
		throw std::invalid_argument("Filter::load: null pointer");
		return *this;
	}
	if (aWidth <= 0 || aHeight <= 0) {
		throw std::invalid_argument("Filter::load: invalid width/height");
		return *this;
	}
	/* load to device memory */
	const size_t pixels = aWidth * aHeight;
	delImage(self->image);
	self->image = newImage(pixels);
	if (self->image == NULL) throw std::bad_alloc();
	self->width = aWidth;
	self->height = aHeight;
	memcpy(self->image, image, sizeof(pcf::color::Rgb32) * pixels);
	/* change channel order to RGBA */
	if (format != RGBA) {
		for (size_t i = 0; i < pixels; i++) {
			const pcf::color::Rgb32 color = self->image[i];
			self->image[i] = pcf::color::Rgb32(color.blue(), color.green(), color.red(), color.alpha());
		}
	}
	return *this;
}


/**
 * Stores the internal image in the given target image. Each target
 * image dimension needs to be at least as big as the source image
 * dimension.
 *
 * @param[out] image - target image buffer
 * @param[in] width - target image width
 * @param[in] height - target image height
 * @param[in] format - target image format
 * @return own instance reference for chained operation
 */
Filter & Filter::store(unsigned char * image, const size_t aWidth, const size_t aHeight, const ImageFormat format) {
	if (self->image == NULL || image == NULL) {
		throw std::invalid_argument("Filter::store: null pointer");
		return *this;
	}
	if (aWidth != width() || aHeight != height()) {
		throw std::invalid_argument("Filter::store: invalid width/height");
		return *this;
	}
	const size_t pixels = aWidth * aHeight;
	pcf::color::Rgb32 * destImage = reinterpret_cast<pcf::color::Rgb32 *>(image);
	if (format != RGBA) {
		/* change channel order to target format */
		for (size_t i = 0; i < pixels; i++) {
			const pcf::color::Rgb32 color = self->image[i];
			destImage[i] = pcf::color::Rgb32(color.blue(), color.green(), color.red(), color.alpha());
		}
	} else {
		/* copy as it */
		for (size_t i = 0; i < pixels; i++) {
			destImage[i] = self->image[i];
		}
	}
	return *this;
}


/**
 * Transforms the internal image to gray scale. The alpha channel is retained.
 *
 * @return own instance reference for chained operation
 */
Filter & Filter::gray() {
	const size_t pixels = self->width * self->height;
	for (size_t i = 0; i < pixels; i++) {
		const pcf::color::Rgb32 color = self->image[i];
		const uint8_t grayVal = uint8_t((float(color.red()) * 0.299f) + (float(color.green()) * 0.587f) + (float(color.blue()) * 0.114f) + 0.5f);
		self->image[i] = pcf::color::Rgb32(grayVal, grayVal, grayVal, color.alpha());
	}
	return *this;
}


/**
 * Transforms the internal image to its inverse colors. The alpha channel is retained.
 *
 * @return own instance reference for chained operation
 */
Filter & Filter::invert() {
	const size_t pixels = self->width * self->height;
	for (size_t i = 0; i < pixels; i++) {
		const pcf::color::Rgb32 color = self->image[i];
		self->image[i] = pcf::color::Rgb32(
			uint8_t(255 - color.red()),
			uint8_t(255 - color.green()),
			uint8_t(255 - color.blue()),
			color.alpha()
		);
	}
	return *this;
}


/**
 * Transforms the internal image to the given color. The alpha channel is retained.
 *
 * @param[in] val - use this color
 * @return own instance reference for chained operation
 */
Filter & Filter::colorize(const pcf::color::SplitColor & val) {
	const float c[3] = {val.value[0], val.value[1], val.value[2]};
	const size_t pixels = self->width * self->height;
	for (size_t i = 0; i < pixels; i++) {
		const pcf::color::Rgb32 color = self->image[i];
		const float grayVal = (float(color.red()) * 0.299f) + (float(color.green()) * 0.587f) + (float(color.blue()) * 0.114f);
		self->image[i] = pcf::color::Rgb32(
			uint8_t((c[0] * grayVal) + 0.5f),
			uint8_t((c[1] * grayVal) + 0.5f),
			uint8_t((c[2] * grayVal) + 0.5f),
			color.alpha()
		);
	}
	return *this;
}


/**
 * Blends the stored image with the given color according to the alpha channel of the
 * stored image.
 *
 * @param[in] val - use this color
 * @return own instance reference for chained operation
 */
Filter & Filter::blend(const pcf::color::SplitColor & val) {
	const float c[3] = {val.value[0], val.value[1], val.value[2]};
	auto mix = [](const float x, const float y, const float a) { return x + ((y - x) * a); };
	const size_t pixels = self->width * self->height;
	for (size_t i = 0; i < pixels; i++) {
		const pcf::color::Rgb32 color = self->image[i];
		const float alpha = float(color.alpha()) / 255.0f;
		const uint8_t rgb[3] = {
			uint8_t((255.0f * mix(c[0], float(color.red()) / 255.0f, alpha)) + 0.5f),
			uint8_t((255.0f * mix(c[1], float(color.green()) / 255.0f, alpha)) + 0.5f),
			uint8_t((255.0f * mix(c[2], float(color.blue()) / 255.0f, alpha)) + 0.5f)
		};
		self->image[i] = pcf::color::Rgb32(rgb[0], rgb[1], rgb[2], 0);
	}
	return *this;
}


} /* namespace image */
} /* namespace pcf */
