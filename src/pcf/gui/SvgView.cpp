/**
 * @file SvgView.cpp
 * @author Daniel Starke
 * @date 2017-08-02
 * @version 2026-06-15
 */
#include <cstdlib>
#include <FL/fl_draw.H>
#include <pcf/gui/SvgView.hpp>
#include <pcf/gui/Utility.hpp>


namespace pcf {
namespace gui {


/**
 * Comparison operator.
 *
 * @param[in] rhs - right-hand side
 * @return true if different, else false
 */
bool SvgView::DrawingStyle::operator!= (const SvgView::DrawingStyle & rhs) const {
	if (this->flags != rhs.flags) return true;
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
SvgView::SvgView(const int X, const int Y, const int W, const int H, const char * L):
	Fl_Widget(X, Y, W, H),
	svg(L),
	blended(NULL),
	blendedSize(0),
	drawingStyle(0, 0, 0, true)
{
	box(FL_FLAT_BOX);
	selection_color(FL_SELECTION_COLOR);
}


/**
 * Destructor.
 */
SvgView::~SvgView() {
	if (this->blended != NULL) free(this->blended);
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int SvgView::handle(int e) {
	switch (e) {
	case FL_ENTER:
	case FL_LEAVE:
		return 1; /* needed to enable tooltip support */
	default:
		return 0;
	}
}


/**
 * Draws the view. This should never be called directly. Use redraw() instead.
 */
void SvgView::draw() {
	if (w() <= 0 || h() <= 0 || !visible()) return;
	const Fl_Color bgColor = color();
	const Fl_Boxtype b = box();
	const int dx = x() + Fl::box_dx(b);
	const int dy = y() + Fl::box_dy(b);
	const int dw = w() - Fl::box_dw(b);
	const int dh = h() - Fl::box_dh(b);
	draw_box(box(), bgColor);
	/* check drawing style */
	DrawingStyle newStyle(flags(), bgColor, 0, false);
	bool colorize = false;
	if ( active() ) {
		if ( colorView() ) {
			newStyle.fgColor = selection_color();
			colorize = true;
		}
	} else {
		newStyle.fgColor = FL_INACTIVE_COLOR;
		colorize = true;
	}
	/* render SVG */
	unsigned char * img = svg.render(size_t(dw), size_t(dh));
	if (img == NULL) return;
	if (svg.redrawn() || newStyle != drawingStyle) {
		/* use cached blending or blend with background and apply effects on change */
		const size_t newSize = size_t(dw) * size_t(dh) * 4;
		if (this->blended == NULL || this->blendedSize != newSize) {
			if (this->blended != NULL) free(this->blended);
			this->blended = static_cast<unsigned char *>(malloc(newSize));
			if (this->blended == NULL) return;
			this->blendedSize = newSize;
		}
		filter.load(img, size_t(dw), size_t(dh));
		if ( colorize ) filter.colorize(pcf::color::SplitColor(newStyle.fgColor));
		filter
			.blend(pcf::color::SplitColor(bgColor))
			.store(this->blended, size_t(dw), size_t(dh))
		;
		drawingStyle = newStyle;
	}
	if (this->blended == NULL) return;
	fl_draw_image(static_cast<uchar *>(this->blended), dx, dy, dw, dh, 4, dw * 4);
}


} /* namespace gui */
} /* namespace pcf */
