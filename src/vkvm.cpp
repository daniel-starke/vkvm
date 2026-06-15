/**
 * @file vkvm.cpp
 * @author Daniel Starke
 * @date 2019-09-30
 * @version 2026-06-14
 */
#include <cstdlib>
#include <stdexcept>
#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <libpcf/tchar.h>
#include <pcf/gui/Utility.hpp>
#include <pcf/gui/VkvmControl.hpp>
#include <pcf/Utility.hpp>

#ifdef PCF_IS_LINUX
#include <pcf/UtilityLinux.hpp>
#endif /* PCF_IS_LINUX */


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
