/**
 * @file HoverButton.hpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_HOVERBUTTON_HPP__
#define __PCF_GUI_HOVERBUTTON_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Button.H>


namespace pcf {
namespace gui {


/**
 * Normal button with additional hover style option.
 */
class HoverButton : public Fl_Button {
private:
	enum Flag {
		HOVER = USERFLAG1
	};
public:
	explicit HoverButton(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~HoverButton();

	inline bool hover() const { return flags() & static_cast<unsigned int>(HOVER); }
	inline void hover(const bool val) { updateStyle(static_cast<unsigned int>(HOVER), val); }
protected:
	virtual int handle(int e);
	virtual void draw();
	inline void updateStyle(const unsigned int f, const bool on) {
		unsigned int oldFlags = flags();
		on ? set_flag(f) : clear_flag(f);
		if (oldFlags != flags()) redraw();
	}
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_HOVERBUTTON_HPP__ */
