/**
 * @file Utility.hpp
 * @author Daniel Starke
 * @date 2019-09-30
 * @version 2023-10-03
 */
#ifndef __PCF_GUI_UTILITY_HPP__
#define __PCF_GUI_UTILITY_HPP__

#include <FL/Fl.H>


namespace pcf {
namespace gui {


/** Label type to disable symbols. Initialize with Fl::set_labeltype(FL_NO_SYMBOL_LABEL, noSymLabelDraw, noSymLabelMeasure) */
#define FL_NO_SYMBOL_LABEL FL_FREE_LABELTYPE


/**
 * Binds the given class function with the passed widget type.
 *
 * @param[in] class - current class
 * @param[in] func - function to bind
 * @param[in] type - widget type
 */
#define PCF_GUI_BIND(class, func, type) \
	static inline void func##_callback(Fl_Widget * w, void * user) { \
		if (w == NULL || user == NULL) return; \
		type * widget = static_cast<type *>(w); (void)widget; \
		class * self = static_cast<class *>(user); \
		self->func(widget); \
	}


/**
 * Returns the binding function for the given function binding.
 *
 * @param[in] func - function bound
 */
#define PCF_GUI_CALLBACK(func) func##_callback


void noSymLabelDraw(const Fl_Label * o, int X, int Y, int W, int H, Fl_Align align);
void noSymLabelMeasure(const Fl_Label * o, int & W, int & H);
int adjDpiH(const int val, const int screen = 0);
int adjDpiV(const int val, const int screen = 0);


/**
 * Interface for linked hover style widgets.
 */
class LinkedHoverState {
protected:
	LinkedHoverState * hoverLink;
	bool hovered;
public:
	explicit inline LinkedHoverState(LinkedHoverState * hl = NULL): hoverLink(hl), hovered(false) {}
	virtual ~LinkedHoverState() {}

	void linkHoverState(LinkedHoverState * dst = NULL);
	void updateHoverState(const bool isHover);
protected:
	/**
	 * Called if the hover state was updated.
	 *
	 * @param[in,out] src - either this object or hoverLink
	 */
	virtual void updateHoverState(LinkedHoverState & src) = 0;
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_UTILITY_HPP__ */
