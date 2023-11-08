/**
 * @file SvgView.hpp
 * @author Daniel Starke
 * @date 2017-08-02
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_SVGVIEW_HPP__
#define __PCF_GUI_SVGVIEW_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <pcf/image/Filter.hpp>
#include <pcf/image/Svg.hpp>


namespace pcf {
namespace gui {


/**
 * View which uses the given SVG data to render an image.
 * Optionally replace all SVG foreground colors with a given one (0 = disable by default).
 */
class SvgView : public Fl_Widget {
private:
	enum Flag {
		COLOR_VIEW = USERFLAG1
	};
	pcf::image::SvgRenderer svg;
	pcf::image::Filter filter;
	struct DrawingStyle {
		unsigned int flags;
		Fl_Color bgColor;
		Fl_Color fgColor;
		bool forcing;
		explicit inline DrawingStyle(const unsigned int fl = 0, const Fl_Color bg = 0, const Fl_Color fg = 0, const bool f = false):
			flags(fl),
			bgColor(bg),
			fgColor(fg),
			forcing(f)
		{}
		bool operator!= (const DrawingStyle & rhs) const;
	} drawingStyle;
public:
	explicit SvgView(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~SvgView();

	inline bool colorView() const { return flags() & static_cast<unsigned int>(COLOR_VIEW); }
	inline void colorView(const bool val) { updateStyle(static_cast<unsigned int>(COLOR_VIEW), val); }

	inline void label(const char * L) { this->svg.data(L); }
protected:
	virtual int handle(int e);
	virtual void draw();
	inline void updateStyle(const unsigned int f, const bool on) {
		unsigned int oldFlags = flags();
		on ? set_flag(f) : clear_flag(f);
		if (oldFlags != flags()) redraw();
	}
private:
	inline const char * label() const { return NULL; }
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_SVGVIEW_HPP__ */
