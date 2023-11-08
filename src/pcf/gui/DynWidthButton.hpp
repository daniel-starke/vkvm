/**
 * @file DynWidthButton.hpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_DYNWIDTHBUTTON_HPP__
#define __PCF_GUI_DYNWIDTHBUTTON_HPP__

#include <pcf/gui/HoverButton.hpp>


namespace pcf {
namespace gui {


/**
 * Button which automatically scales the with to the label set.
 * Enable hover style buttons if desired.
 */
class DynWidthButton : public pcf::gui::HoverButton {
public:
	explicit DynWidthButton(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~DynWidthButton();

	inline void label(const char * text) {
		Fl_Button::label(text);
		updateWidth();
	}
	inline void label(Fl_Labeltype a, const char * b) {
		Fl_Button::label(a, b);
		updateWidth();
	}
protected:
	void updateWidth();
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_DYNWIDTHBUTTON_HPP__ */
