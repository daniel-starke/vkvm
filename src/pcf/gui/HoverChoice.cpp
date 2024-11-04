/**
 * @file HoverChoice.cpp
 * @author Daniel Starke
 * @date 2019-10-14
 * @version 2024-11-04
 */
#include <FL/fl_draw.H>
#include <pcf/gui/HoverChoice.hpp>


namespace pcf {
namespace gui {


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - label
 */
HoverChoice::HoverChoice(const int X, const int Y, const int W, const int H, const char * L):
	Fl_Menu_(X, Y, W, H, L)
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
HoverChoice::~HoverChoice() {}


/**
 * Sets the currently selected value using the index into the menu item array.
 * Changing the selected value causes a redraw().
 * @param[in] v - index of value in the menu item array.
 * @return non-zero if the new value is different to the old one.
 */
int HoverChoice::value(const int v) {
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
int HoverChoice::value(const Fl_Menu_Item * v) {
	if ( ! Fl_Menu_::value(v) ) return 0;
	redraw();
	return 1;
}


/**
 * Adds the given item label without special character escaping.
 * Internally all special characters are escapes to ensure that the
 * given label is being used without any special meanings.
 * Nevertheless, `@` is passed as it. Use `noSymLabelDraw()` and `noSymLabelMeasure()`
 * to control the behavior for it.
 *
 * @param[in] label - raw text label for the menu item
 * @param[in] shortcut - optional keyboard shortcut that can be an int or string: (FL_CTRL+'a') or "^a" (default 0 if none)
 * @param[in] callback - optional callback invoked when user clicks the item (default 0 if none)
 * @param[in] userData - optional user data passed as an argument to the callback (default 0 if none)
 * @param[in] flags - optional flags that control the type of menu item (default 0 if none, see `Fl_Menu_::add()`)
 * @return the index into the menu() array, where the entry was added
 */
int HoverChoice::addRaw(const char * label, int shortcut, Fl_Callback * callback, void * userData, int flags) {
	size_t len = 0;
	for (const char * ptr = label; *ptr != 0; ptr++, len++) {
		switch (*ptr) {
		case '&':
		case '/':
		case '\\':
		case '_':
			len++; /* need escaping */
			break;
		default:
			break;
		}
	}
	char * buf = static_cast<char *>(malloc(sizeof(char) * (len + 1)));
	if (buf == NULL) {
		return -1;
	}
	char * out = buf;
	for (const char * ptr = label; *ptr != 0; ptr++) {
		switch (*ptr) {
		case '&':
			*out = '&'; /* no underline */
			out++;
			break;
		case '/':
		case '\\':
		case '_':
			*out = '\\'; /* no separator and sub menu path handling */
			out++;
			break;
		default:
			break;
		}
		*out = *ptr;
		out++;
	}
	*out = 0;
	const int res = this->add(buf, shortcut, callback, userData, flags);
	free(buf);
	return res;
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int HoverChoice::handle(int e) {
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
		if (fl_contrast(textcolor(), FL_BACKGROUND2_COLOR) != textcolor()) {
			v = menu()->pulldown(x(), y(), w(), h(), menu(), this);
		} else {
			v = menu()->pulldown(x(), y(), w(), h(), menu(), this);
		}
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
void HoverChoice::draw() {
	if (w() <= 0 || h() <= 0 || !visible()) return;
	const bool pushedAndHover = (Fl::belowmouse() == this && Fl::pushed() == this);
	const Fl_Color bgColor = pushedAndHover ? fl_color_average(color(), FL_FOREGROUND_COLOR, 0.8f) : color();
	const Fl_Boxtype b = box();
	const int X = x();
	const int Y = y();
	const int W = w();
	const int H = h();
	const int dx = Fl::box_dx(b);
	const int dy = Fl::box_dy(b);
	/* hover style frame */
	if ( pushedAndHover ) {
		/* selected state */
		draw_box(down_box() ? down_box() : fl_down(b), bgColor);
	} else if (Fl::belowmouse() == this && active()) {
		/* hover state */
		draw_box(pushedAndHover ? (down_box() ? down_box() : fl_down(b)) : b, bgColor);
	} else {
		/* normal state */
		fl_rectf(X, Y, W, H, bgColor);
	}
	/* render text */
	if (mvalue() == NULL) return;
	Fl_Menu_Item m = *mvalue();
	/* draw clipped */
	const int xx = X + dx;
	const int yy = Y + dy;
	const int ww = W - (2 * dx) - 2;
	const int hh = H - (2 * dy) - 2;
	fl_push_clip(xx, yy, ww, hh);
	fl_color(active_r() ? labelcolor() : fl_inactive(labelcolor()));
	m.draw(xx, yy, ww, hh, this, 0);
	fl_pop_clip();
	box(b);
}


} /* namespace gui */
} /* namespace pcf */
