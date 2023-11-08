/**
 * @file HoverDropDown.hpp
 * @author Daniel Starke
 * @date 2019-11-23
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_HOVERDROPDOWN_HPP__
#define __PCF_GUI_HOVERDROPDOWN_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>


namespace pcf {
namespace gui {


/**
 * Hover drop down menu. The hover style is always set.
 */
class HoverDropDown : public Fl_Menu_ {
public:
	explicit HoverDropDown();

	virtual ~HoverDropDown();

	inline int value() const { return Fl_Menu_::value(); }
	int value(const int v);
	int value(const Fl_Menu_Item * v);

	const Fl_Menu_Item * dropDown(int X, int Y, int W = 0, int H = 0);

	virtual int handle(int e);
protected:
	virtual void draw();
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_HOVERDROPDOWN_HPP__ */
