/**
 * @file SvgButton.cpp
 * @author Daniel Starke
 * @date 2017-04-09
 * @version 2020-05-02
 */
#include <FL/fl_draw.H>
#include <pcf/gui/SvgButton.hpp>
#include <pcf/gui/Utility.hpp>


namespace pcf {
namespace gui {


/**
 * Comparison operator.
 *
 * @param[in] rhs - right-hand side
 * @return true if different, else false
 */
bool SvgButton::DrawingStyle::operator!= (const SvgButton::DrawingStyle & rhs) const {
	if (this->flags != rhs.flags) return true;
	if (this->type != rhs.type) return true;
	if (this->bgColor != rhs.bgColor) return true;
	if (this->fgColor != rhs.fgColor) return true;
	if (this->forcing != rhs.forcing) return true;
	return false;
}


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - SVG image data
 */
SvgButton::SvgButton(const int X, const int Y, const int W, const int H, const char * L):
	Fl_Button(X, Y, W, H),
	svg(L),
	drawingStyle(0, 0, 0, 0, true)
{
	box(FL_THIN_UP_BOX);
	selection_color(FL_SELECTION_COLOR);
}


/**
 * Destructor.
 */
SvgButton::~SvgButton() {}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int SvgButton::handle(int e) {
	int result = Fl_Button::handle(e);
	switch (e) {
	case FL_ENTER:
	case FL_LEAVE:
		this->LinkedHoverState::updateHoverState(e == FL_ENTER);
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
void SvgButton::draw() {
	if (type() == FL_HIDDEN_BUTTON) return;
	if (w() <= 0 || h() <= 0 || !visible()) return;
	const bool small = hover() ? (value() && (Fl::pushed() == this)) : value();
	const Fl_Color bgColor = value() ? fl_color_average(color(), FL_FOREGROUND_COLOR, 0.8f) : color();
	const Fl_Boxtype b = box();
	const int dx = x() + Fl::box_dx(b) + (small ? 1 : 0);
	const int dy = y() + Fl::box_dy(b) + (small ? 1 : 0);
	const int dw = w() - Fl::box_dw(b) - (small ? 2 : 1);
	const int dh = h() - Fl::box_dh(b) - (small ? 2 : 1);
	if ( hover() ) {
		/* hover style button */
		if ( value() ) {
			/* selected state */
			draw_box(down_box() ? down_box() : fl_down(box()), bgColor);
		} else if ( this->hovered ) {
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
	/* check drawing style */
	DrawingStyle newStyle(flags(), type(), bgColor, 0, false);
	bool colorize = false;
	if ( active() ) {
		if ( colorButton() ) {
			switch (type()) {
			case FL_TOGGLE_BUTTON:
			case FL_RADIO_BUTTON:
				if (value() && Fl::pushed() != this) {
					newStyle.fgColor = selection_color();
					break;
				}
				/* FALLTHROUGH */
			case FL_NORMAL_BUTTON:
			default:
				newStyle.fgColor = labelcolor();
				break;
			}
			colorize = true;
		}
	} else {
		newStyle.fgColor = FL_INACTIVE_COLOR;
		colorize = true;
	}
	/* render SVG */
	unsigned char * img = NULL;
	if (newStyle != drawingStyle) {
		img = svg.render(size_t(dw), size_t(dh), true);
		drawingStyle = newStyle;
	} else {
		img = svg.render(size_t(dw), size_t(dh));
	}
	if (img == NULL) return;
	/* blend with background and apply effects */
	if ( svg.redrawn() ) {
		filter.load(img, size_t(dw), size_t(dh));
		if ( colorize ) filter.colorize(pcf::color::SplitColor(newStyle.fgColor));
		filter
			.blend(pcf::color::SplitColor(bgColor))
			.store(img, size_t(dw), size_t(dh))
		;
	}
	fl_draw_image(static_cast<uchar *>(img), dx, dy, dw, dh, 4, dw * 4);
	if (!hover() && Fl::focus() == this) draw_focus();
}


/**
 * Called to update the hover state of this widget.
 *
 * @param[in,out] src - either this object or hoverLink
 */
void SvgButton::updateHoverState(LinkedHoverState & src) {
	this->redraw();
}


} /* namespace gui */
} /* namespace pcf */
