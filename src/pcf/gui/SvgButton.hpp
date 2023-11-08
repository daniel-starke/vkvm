/**
 * @file SvgButton.hpp
 * @author Daniel Starke
 * @date 2017-04-09
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_SVGBUTTON_HPP__
#define __PCF_GUI_SVGBUTTON_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Button.H>
#include <pcf/image/Filter.hpp>
#include <pcf/image/Svg.hpp>
#include <pcf/gui/Utility.hpp>


namespace pcf {
namespace gui {


/**
 * Button which uses the given SVG data to render an image.
 * Optionally replace all SVG foreground colors with a one given via selection_color() (0 = disable by default).
 * Enable hover style buttons if desired.
 */
class SvgButton : public Fl_Button, public LinkedHoverState {
private:
	enum Flag {
		HOVER = USERFLAG1,
		COLOR_BUTTON = USERFLAG2
	};
	pcf::image::SvgRenderer svg;
	pcf::image::Filter filter;
	struct DrawingStyle {
		unsigned int flags;
		uchar type;
		Fl_Color bgColor;
		Fl_Color fgColor;
		bool forcing;
		explicit inline DrawingStyle(const unsigned int fl = 0, const uchar t = 0, const Fl_Color bg = 0, const Fl_Color fg = 0, const bool f = false):
			flags(fl),
			type(t),
			bgColor(bg),
			fgColor(fg),
			forcing(f)
		{}
		bool operator!= (const DrawingStyle & rhs) const;
	} drawingStyle;
public:
	explicit SvgButton(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~SvgButton();

	inline bool hover() const { return flags() & static_cast<unsigned int>(HOVER); }
	inline void hover(const bool val) { updateStyle(static_cast<unsigned int>(HOVER), val); }
	inline bool colorButton() const { return flags() & static_cast<unsigned int>(COLOR_BUTTON); }
	inline void colorButton(const bool val) { updateStyle(static_cast<unsigned int>(COLOR_BUTTON), val); }

	inline void label(const char * L) { this->svg.data(L); }
protected:
	virtual int handle(int e);
	virtual void draw();
	virtual void updateHoverState(LinkedHoverState & src);
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


#endif /* __PCF_GUI_SVGBUTTON_HPP__ */
