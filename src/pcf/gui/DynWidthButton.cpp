/**
 * @file DynWidthButton.cpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2019-10-14
 */
#include <FL/fl_draw.H>
#include <pcf/gui/DynWidthButton.hpp>


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
DynWidthButton::DynWidthButton(const int X, const int Y, const int W, const int H, const char * L):
	pcf::gui::HoverButton(X, Y, W, H, L)
{
	box(FL_THIN_UP_BOX);
	selection_color(FL_SELECTION_COLOR);
	updateWidth();
}


/**
 * Destructor.
 */
DynWidthButton::~DynWidthButton() {}


/**
 * Updates the width of the button according to the current label text and font.
 */
void DynWidthButton::updateWidth() {
	fl_font(labelfont(), labelsize());
	const Fl_Boxtype b = box();
	size(int(fl_width(Fl_Widget::label())) + Fl::box_dx(b) + Fl::box_dw(b) + 4, h());
}


} /* namespace gui */
} /* namespace pcf */
