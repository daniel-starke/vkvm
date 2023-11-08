/**
 * @file ScrollableValueInput.hpp
 * @author Daniel Starke
 * @date 2020-04-23
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_SCROLLABLEVALUEINPUT_HPP__
#define __PCF_GUI_SCROLLABLEVALUEINPUT_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Value_Input.H>


namespace pcf {
namespace gui {


/**
 * Fl_Value_Input which can additionally be manipulated via mouse wheel.
 */
class ScrollableValueInput : public Fl_Value_Input {
private:
	enum Flag {
		ON_FOCUS = USERFLAG1 /**< on hover if unset */
	};
public:
	explicit ScrollableValueInput(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~ScrollableValueInput();

	inline bool onHover() const { return !(flags() & static_cast<unsigned int>(ON_FOCUS)); }
	inline void onHover(const bool val) { updateFlag(static_cast<unsigned int>(ON_FOCUS), !val); }
	inline bool onFocus() const { return flags() & static_cast<unsigned int>(ON_FOCUS); }
	inline void onFocus(const bool val) { updateFlag(static_cast<unsigned int>(ON_FOCUS), val); }
protected:
	virtual int handle(int e);
	inline void updateFlag(const unsigned int f, const bool on) {
		on ? set_flag(f) : clear_flag(f);
	}
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_SCROLLABLEVALUEINPUT_HPP__ */
