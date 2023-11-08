/**
 * @file ScrollableValueInput.cpp
 * @author Daniel Starke
 * @date 2020-04-23
 * @version 2020-04-23
 */
#include <pcf/gui/ScrollableValueInput.hpp>


namespace pcf {
namespace gui {


/**
 * Constructor.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - width
 * @param[in] H - height
 * @param[in] L - label (optional)
 */
ScrollableValueInput::ScrollableValueInput(const int X, const int Y, const int W, const int H, const char * L) : Fl_Value_Input(X, Y, W, H, L) {
}


/**
 * Destructor.
 */
ScrollableValueInput::~ScrollableValueInput() {
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int ScrollableValueInput::handle(int e) {
	int result = 0;
	switch (e) {
	case FL_MOUSEWHEEL:
		if ((onHover() && Fl::event_inside(&input)) || (onFocus() && Fl::focus() == &input)){
			const int moved(-Fl::event_dy());
			if (moved != 0) {
				const double newValue(clamp(value() + (double(moved) * step())));
				if (value(newValue) && (when() & FL_WHEN_CHANGED)) do_callback();
			}
			result = 1;
		}
		break;
	default:
		result = Fl_Value_Input::handle(e);
		if (e == FL_FOCUS || e == FL_UNFOCUS) {
			/* make current value a multiple of step and fit it into range */
			handle_push();
			double v = increment(previous_value(), 0);
			v = round(v);
			handle_drag(soft() ? softclamp(v) : clamp(v));
			handle_release();
		}
		break;
	}
	return result;
}


} /* namespace gui */
} /* namespace pcf */
