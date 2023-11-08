/**
 * @file Filter.hpp
 * @author Daniel Starke
 * @date 2017-11-09
 * @version 2019-10-10
 */
#ifndef __PCF_IMAGE_FILTER_HPP__
#define __PCF_IMAGE_FILTER_HPP__

#include <cstddef>
#include <pcf/color/SplitColor.hpp>


namespace pcf {
namespace image {


struct FilterData;


/**
 * Renderer which uses the given filter data to render an image as RGBA32 byte array.
 */
class Filter {
public:
	enum ImageFormat {
		RGBA,
		BGRA
	};
private:
	FilterData * self;
public:
	explicit Filter();
	Filter(const Filter & o);
	~Filter();

	Filter & operator= (const Filter & o);

	size_t width() const;
	size_t height() const;

	Filter & clear();
	Filter & load(const unsigned char * image, const size_t aWidth, const size_t aHeight, const ImageFormat format = RGBA);
	Filter & store(unsigned char * image, const size_t aWidth, const size_t aHeight, const ImageFormat format = RGBA);

	Filter & gray();
	Filter & invert();
	Filter & colorize(const pcf::color::SplitColor & val);
	Filter & blend(const pcf::color::SplitColor & val);
};


} /* namespace image */
} /* namespace pcf */


#endif /* __PCF_IMAGE_FILTER_HPP__ */
