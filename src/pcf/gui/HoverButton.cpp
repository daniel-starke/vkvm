/**
 * @file HoverButton.cpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2020-05-02
 */
#include <FL/fl_draw.H>
#include <pcf/gui/HoverButton.hpp>


namespace pcf {
namespace gui {


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - SVG image data
 */
HoverButton::HoverButton(const int X, const int Y, const int W, const int H, const char * L):
	Fl_Button(X, Y, W, H, L)
{
	box(FL_THIN_UP_BOX);
	selection_color(FL_SELECTION_COLOR);
}


/**
 * Destructor.
 */
HoverButton::~HoverButton() {}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int HoverButton::handle(int e) {
	int result = Fl_Button::handle(e);
	switch (e) {
	case FL_ENTER:
	case FL_LEAVE:
		redraw();
		break;
	case FL_KEYBOARD:
		switch (Fl::event_key()) {
		case FL_KP_Enter:
		case FL_Enter:
			this->do_callback();
			result = 1;
			break;
		default:
			break;
		}
		break;
	case FL_RELEASE:
		Fl::pushed(NULL);
		redraw();
		break;
	default:
		break;
	}
	return result;
}


/**
 * Draws the button. This should never be called directly. Use redraw() instead.
 */
void HoverButton::draw() {
	if (type() == FL_HIDDEN_BUTTON) return;
	if (w() <= 0 || h() <= 0 || !visible()) return;
	const bool small = hover() ? (value() && (Fl::pushed() == this)) : value();
	const Fl_Color bgColor = value() ? fl_color_average(color(), FL_FOREGROUND_COLOR, 0.8f) : color();
	if ( hover() ) {
		/* hover style button */
		if ( value() ) {
			/* selected state */
			draw_box(down_box() ? down_box() : fl_down(box()), bgColor);
		} else if (Fl::belowmouse() == this && active()) {
			/* hover state */
			draw_box(value() ? (down_box() ? down_box() : fl_down(box())) : box(), bgColor);
		} else {
			/* normal state */
			fl_rectf(x(), y(), w(), h(), bgColor);
		}
	} else {
		/* standard button */
		draw_box(small ? (down_box() ? down_box() : fl_down(box())) : box(), bgColor);
	}
	/* render text */
	if (labeltype() == FL_NORMAL_LABEL && value()) {
		const Fl_Color c = labelcolor();
		labelcolor(fl_contrast(c, bgColor));
		draw_label();
		labelcolor(c);
	} else {
		draw_label();
	}
	if (!hover() && Fl::focus() == this) draw_focus();
}


} /* namespace gui */
} /* namespace pcf */
