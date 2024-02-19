/**
 * @file vkvm.cpp
 * @author Daniel Starke
 * @date 2019-09-30
 * @version 2024-02-18
 */
#include <cstdlib>
#include <stdexcept>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <libpcf/tchar.h>
#include <pcf/image/Draw.hpp>
#include <pcf/gui/Utility.hpp>
#include <pcf/gui/VkvmControl.hpp>
#include <pcf/Utility.hpp>

#ifdef PCF_IS_LINUX
#include <pcf/UtilityLinux.hpp>
#endif /* PCF_IS_LINUX */


/**
 * Custom function for FL_ROUND_DOWN_BOX to get a different style for radio buttons.
 *
 * @param[in] x - x offset
 * @param[in] y - y offset
 * @param[in] w - width
 * @param[in] h - height
 * @param[in] bgcolor - background color (FL_FOREGROUND_COLOR is used as foreground color)
 */
static void customRoundDownBox(int x, int y, int w, int h, Fl_Color bgcolor) {
	/* get buffer */
	uchar * img = fl_read_image(NULL, x, y, w, h);
	if (img == NULL) return;
	const auto setColor = [=](const size_t xo, const size_t yo, const pcf::color::SplitColor & fg) {
		if (xo >= size_t(w) || yo >= size_t(h)) return;
		const size_t offset = size_t(((yo * size_t(w)) + xo) * 3);
		const Fl_Color bgColor = Fl_Color(((((img[offset + 2] << 8) | img[offset + 1]) << 8) | img[offset]) << 8);
		const pcf::color::SplitColor bg(bgColor);
		const Fl_Color newColor = static_cast<Fl_Color>(pcf::image::blendOver(fg, bg));
		img[offset] = uchar((newColor >> 8) & 0xFF);
		img[offset + 1] = uchar((newColor >> 16) & 0xFF);
		img[offset + 2] = uchar((newColor >> 24) & 0xFF);
	};
	const int t = 2;
	const int t2 = t * 2;
	const int mT = (w < h) ? w : h;
	/* draw filled area */
	pcf::image::drawEllipseAA(setColor, size_t(w / 2), size_t(h / 2), size_t(w - t2), size_t(h - t2), size_t((mT - t2) / 2), pcf::color::SplitColor(bgcolor));
	/* draw outline */
	pcf::image::drawEllipseAA(setColor, size_t(w / 2), size_t(h / 2), size_t(w - t), size_t(h - t), size_t(t - 1), pcf::color::SplitColor(FL_FOREGROUND_COLOR));
	/* update from buffer */
	fl_draw_image(img, x, y, w, h);
	delete [] img;
}


/** Unicode compatible main entry point. */
#ifndef PCF_IS_LINUX
int _tmain() {
#ifdef PCF_IS_WIN
	SetProcessDPIAware();
#endif /* PCF_IS_WIN */
#else /* PCF_IS_LINUX */
int _tmain(int argc, TCHAR * argv[]) {
#endif /* PCF_IS_LINUX */
	try {
		Fl_Widget * dialogIcon = fl_message_icon();
		if (dialogIcon != NULL) {
			dialogIcon->label("!");
			dialogIcon->labelcolor(FL_RED);
		}
		Fl::visual(FL_DOUBLE | FL_RGB);
		Fl::set_color(FL_BACKGROUND_COLOR, 212, 208, 200);
		Fl::set_labeltype(FL_NO_SYMBOL_LABEL, pcf::gui::noSymLabelDraw, pcf::gui::noSymLabelMeasure);
		Fl::set_boxtype(FL_ROUND_DOWN_BOX, customRoundDownBox, 2, 2, 4, 4); /* also reduces executable size */
		FL_NORMAL_SIZE = pcf::gui::adjDpiV(FL_NORMAL_SIZE);
		Fl::lock(); /* enable FLTK multi-threading mechanism (needs to be called latest here) */
#ifdef PCF_IS_LINUX
		if ( ! requestRootPermission(argc, argv) ) {
			return EXIT_FAILURE;
		}
#endif /* PCF_IS_LINUX */
		pcf::gui::VkvmControl * window = new pcf::gui::VkvmControl(pcf::gui::adjDpiH(640), pcf::gui::adjDpiV(534), "vkvm " VKVM_VERSION);
		window->show();
		return Fl::run();
	} catch (const std::exception & e) {
		fl_message_title("Error");
		fl_alert("Exception: %s", e.what());
		return EXIT_FAILURE;
	}
}
