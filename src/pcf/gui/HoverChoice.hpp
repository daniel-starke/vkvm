/**
 * @file HoverChoice.hpp
 * @author Daniel Starke
 * @date 2019-10-14
 * @version 2024-11-04
 */
#ifndef __PCF_GUI_HOVERCHOICE_HPP__
#define __PCF_GUI_HOVERCHOICE_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>


namespace pcf {
namespace gui {


/**
 * Hover choice button. The hover style is always set.
 */
class HoverChoice : public Fl_Menu_ {
public:
	explicit HoverChoice(const int X, const int Y, const int W, const int H, const char * L = NULL);

	virtual ~HoverChoice();

	inline int value() const { return Fl_Menu_::value(); }
	int value(const int v);
	int value(const Fl_Menu_Item * v);

	int addRaw(const char * label, int shortcut, Fl_Callback * callback, void * userData = NULL, int flags = 0);

	virtual int handle(int e);
protected:
	virtual void draw();
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_HOVERCHOICE_HPP__ */
