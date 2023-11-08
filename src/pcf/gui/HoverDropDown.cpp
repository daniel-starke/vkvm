/**
 * @file HoverDropDown.cpp
 * @author Daniel Starke
 * @date 2019-11-23
 * @version 2023-10-24
 */
#include <FL/fl_draw.H>
#include <pcf/gui/HoverDropDown.hpp>


namespace pcf {
namespace gui {


/**
 * Constructor.
 */
HoverDropDown::HoverDropDown():
	Fl_Menu_(0, 0, 0, 0, NULL)
{
	align(FL_ALIGN_LEFT);
	when(FL_WHEN_RELEASE);
	textfont(FL_HELVETICA);
	down_box(FL_BORDER_BOX);
	box(FL_THIN_UP_BOX);
	selection_color(FL_SELECTION_COLOR);
}


/**
 * Destructor.
 */
HoverDropDown::~HoverDropDown() {}


/**
 * Sets the currently selected value using the index into the menu item array.
 * Changing the selected value causes a redraw().
 * @param[in] v - index of value in the menu item array.
 * @return non-zero if the new value is different to the old one.
 */
int HoverDropDown::value(const int v) {
	if (v == -1) return value(static_cast<const Fl_Menu_Item *>(NULL));
	if (v < 0 || v >= (size() - 1)) return 0;
	if ( ! Fl_Menu_::value(v) ) return 0;
	redraw();
	return 1;
}


/**
 * Sets the currently selected value using a pointer to menu item.
 * Changing the selected value causes a redraw().
 * @param[in] v - pointer to menu item in the menu item array.
 * @return non-zero if the new value is different to the old one.
 */
int HoverDropDown::value(const Fl_Menu_Item * v) {
	if ( ! Fl_Menu_::value(v) ) return 0;
	redraw();
	return 1;
}


/**
 * Display the drop down menu at the given screen coordinated.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - minimal width
 * @param[in] H - minimal height
 * @return selected menu item
 */
const Fl_Menu_Item * HoverDropDown::dropDown(int X, int Y, int W, int H) {
	redraw();
	Fl_Widget_Tracker mb(this);
	const Fl_Menu_Item * m = menu()->pulldown(X, Y, W, H, 0, this);
	picked(m);
	if ( mb.exists() ) redraw();
	return m;
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int HoverDropDown::handle(int e) {
	if (!menu() || !menu()->text) return 0;
	const Fl_Menu_Item * v;
	switch (e) {
	case FL_ENTER:
	case FL_LEAVE:
		redraw();
		return 1;
	case FL_KEYBOARD:
		if (Fl::event_key() != ' ' || (Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META))) return 0;
		/* fall-through */
	case FL_PUSH:
		if (Fl::visible_focus()) Fl::focus(this);
	J1:
		v = menu()->pulldown(x(), y(), w(), h(), menu(), this);
		if (!v || v->submenu()) return 1;
		picked(v);
		Fl::pushed(NULL);
		redraw();
		return 1;
	case FL_SHORTCUT:
		if ( Fl_Widget::test_shortcut() ) goto J1;
		v = menu()->test_shortcut();
		if ( ! v ) return 0;
		picked(v);
		Fl::pushed(NULL);
		redraw();
		return 1;
	case FL_FOCUS:
	case FL_UNFOCUS:
		if ( Fl::visible_focus() ) {
			redraw();
			return 1;
		}
		return 0;
	default:
		return 0;
	}
}


/**
 * Draws the button. This should never be called directly. Use redraw() instead.
 */
void HoverDropDown::draw() {
	/* only the menu gets drawn, hence, nothing to draw as a widget */
}


} /* namespace gui */
} /* namespace pcf */
