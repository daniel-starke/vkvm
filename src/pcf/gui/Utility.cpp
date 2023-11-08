/**
 * @file Utility.cpp
 * @author Daniel Starke
 * @date 2019-09-30
 * @version 2019-11-25
 */
#include <cmath>
#include <cstdlib>
#include <FL/fl_draw.H>
#include <pcf/Utility.hpp>
#include <pcf/gui/Utility.hpp>


/** Defines the maximum number of available screen resolutions (derived from FLTK). */
#define MAX_SCREENS_RES 16


namespace pcf {
namespace gui {


/**
 * Alternative drawing function for text without symbol interpretion.
 *
 * @param[in] o - label instance
 * @param[in] X - x coordinates
 * @param[in] Y - y coordinates
 * @param[in] W - width of the boundary box
 * @param[in] H - height of the boundary box
 * @param[in] align - alignment
 */
void noSymLabelDraw(const Fl_Label * o, int X, int Y, int W, int H, Fl_Align align) {
	fl_font(o->font, o->size);
	fl_color(static_cast<Fl_Color>(o->color));
	fl_draw(o->value, X, Y, W, H, align, o->image, 0);
}


/**
 * Alternative measurement function for text without symbol interpretion.
 *
 * @param[in] o - label instance
 * @param[out] W - calculated width
 * @param[out] H - calculated height
 */
void noSymLabelMeasure(const Fl_Label * o, int & W, int & H) {
	fl_font(o->font, o->size);
	fl_measure(o->value, W, H, 0);
	if ( o->image ) {
		if (o->image->w() > W) W = o->image->w();
		H += o->image->h();
	}
}


/**
 * Helper type to return the screen DPI array from getScreenDpis().
 */
typedef float ScreenDpiArray[MAX_SCREENS_RES][2];


/**
 * Helper function load the screen resolutions at program start.
 *
 * @return Reference to screen DPI array.
 * @remarks Set the environment variables FLTK_DPI_H and FLTK_DPI_V
 *          or just FLTK_DPI to adjust screen DPI manually.
 * @internal
 */
static const ScreenDpiArray & getScreenDpis() {
	static ScreenDpiArray screenDpis;
	static bool hasDpis = false;
	if ( ! hasDpis ) {
		const char * dpiEnvStr = getenv("FLTK_DPI");
		const float dpiEnv = (dpiEnvStr != NULL) ? float(atof(dpiEnvStr)) : 0.0f;
		if (dpiEnv > 0.0f) {
			for (int i = 0; i < MAX_SCREENS_RES; i++) {
				screenDpis[i][0] = dpiEnv;
				screenDpis[i][1] = dpiEnv;
			}
			hasDpis = true;
		}
	}
	if ( ! hasDpis ) {
		const char * dpiEnvStrH = getenv("FLTK_DPI_H");
		const char * dpiEnvStrV = getenv("FLTK_DPI_V");
		const float dpiEnvH = (dpiEnvStrH != NULL) ? float(atof(dpiEnvStrH)) : 0.0f;
		const float dpiEnvV = (dpiEnvStrV != NULL) ? float(atof(dpiEnvStrV)) : 0.0f;
		if (dpiEnvH > 0.0f && dpiEnvV > 0.0f) {
			for (int i = 0; i < MAX_SCREENS_RES; i++) {
				screenDpis[i][0] = dpiEnvH;
				screenDpis[i][1] = dpiEnvV;
			}
			hasDpis = true;
		}
	}
	if ( ! hasDpis ) {
		for (int i = 0; i < MAX_SCREENS_RES; i++) {
			Fl::screen_dpi(screenDpis[i][0], screenDpis[i][1], i);
		}
		hasDpis = true;
	}
	return screenDpis;
}


/**
 * Returns the horizontal width equivalent for 96 DPI in the current display resolution.
 *
 * @param[in] val - adjust this value
 * @param[in] screen - for this screen
 * @see Fl::screen_dpi() and getScreenDpis()
 * @remarks Change to 96 DPI on Linux via xrandr --dpi 96 for example.
 */
int adjDpiH(const int val, const int screen) {
	static const ScreenDpiArray & dpis = getScreenDpis();
	return int(round(float(val) * dpis[(screen > MAX_SCREENS_RES) ? 0 : screen][0] / 96.0f));
}


/**
 * Returns the vertical height equivalent for 96 DPI in the current display resolution.
 *
 * @param[in] val - adjust this value
 * @param[in] screen - for this screen
 * @see Fl::screen_dpi() and getScreenDpis()
 * @remarks Change to 96 DPI on Linux via xrandr --dpi 96 for example.
 */
int adjDpiV(const int val, const int screen) {
	static const ScreenDpiArray & dpis = getScreenDpis();
	return int(round(float(val) * dpis[(screen > MAX_SCREENS_RES) ? 0 : screen][1] / 96.0f));
}


/**
 * Links the given hover link partner to this instance. Any previous link will be removed.
 *
 * @param[in,out] dst - link with this instance or NULL to remove the previous link
 */
void LinkedHoverState::linkHoverState(LinkedHoverState * dst) {
	if (this->hoverLink == dst) return;
	/* remove previous link */
	if (this->hoverLink != NULL) {
		this->hoverLink->hoverLink = NULL;
		this->hoverLink = NULL;
	}
	/* add new link	*/
	if (dst != NULL) {
		dst->hoverLink = this;
		this->hoverLink = dst;
	}
}


/**
 * Updates the hover state of this and the linked widget by calling the overloaded internal method.
 *
 * @param[in] isHover - true if the mouse is over this or the linked widget, else false
 */
void LinkedHoverState::updateHoverState(const bool isHover) {
	this->hovered = isHover;
	if (this->hoverLink != NULL) {
		this->hoverLink->hovered = isHover;
	}
	this->updateHoverState(*this);
	if (this->hoverLink != NULL) {
		this->hoverLink->updateHoverState(*this);
	}
}


} /* namespace gui */
} /* namespace pcf */
