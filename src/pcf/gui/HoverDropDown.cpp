/**
 * @file HoverDropDown.cpp
 * @author Daniel Starke
 * @date 2019-11-23
 * @version 2026-06-14
 */
#include <FL/fl_draw.H>
#include <pcf/color/SplitColor.hpp>
#include <pcf/image/Draw.hpp>
#include <pcf/gui/HoverDropDown.hpp>
#include <pcf/gui/Utility.hpp>


namespace pcf {
namespace gui {


/**
 * Label draw function for FL_IMAGE_VALUE_LABEL. The label value holds an Fl_Image pointer which is
 * drawn vertically centered within the given boundary box.
 */
static void imageValueLabelDraw(const Fl_Label * o, int X, int Y, int W, int H, Fl_Align /* align */) {
	Fl_Image * const img = reinterpret_cast<Fl_Image *>(const_cast<char *>(o->value));
	if (img == NULL) return;
	img->draw(X, Y + ((H - img->h()) / 2));
}


/**
 * Label measure function for FL_IMAGE_VALUE_LABEL.
 */
static void imageValueLabelMeasure(const Fl_Label * o, int & W, int & H) {
	Fl_Image * const img = reinterpret_cast<Fl_Image *>(const_cast<char *>(o->value));
	if (img == NULL) {
		W = 0;
		H = 0;
		return;
	}
	W = img->w();
	H = img->h();
}


/**
 * Constructor.
 */
HoverDropDown::HoverDropDown():
	Fl_Menu_(0, 0, 0, 0, NULL),
	radioOnImage(NULL),
	radioOffImage(NULL),
	radioLabels(NULL),
	radioItems(NULL),
	radioCount(0)
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
HoverDropDown::~HoverDropDown() {
	delete [] radioItems;
	delete [] radioLabels;
	delete radioOnImage;
	delete radioOffImage;
}


/**
 * Sets the currently selected value using the index into the menu item array.
 * Changing the selected value causes a redraw().
 *
 * @param[in] v - index of value in the menu item array.
 * @return non-zero if the new value is different to the old one.
 */
int HoverDropDown::value(const int v) {
	if (v == -1) return value(static_cast<const Fl_Menu_Item *>(NULL));
	if (v < 0 || v >= (size() - 1)) return 0;
	if ( ! Fl_Menu_::value(v) ) return 0;
	if (radioCount > 0) selectRadio(mvalue());
	redraw();
	return 1;
}


/**
 * Sets the currently selected value using a pointer to menu item.
 * Changing the selected value causes a redraw().
 *
 * @param[in] v - pointer to menu item in the menu item array.
 * @return non-zero if the new value is different to the old one.
 */
int HoverDropDown::value(const Fl_Menu_Item * v) {
	if ( ! Fl_Menu_::value(v) ) return 0;
	if (radioCount > 0) selectRadio(v);
	redraw();
	return 1;
}


/**
 * Display the drop down menu at the given screen coordinated.
 *
 * @param[in] X - x coordinate
 * @param[in] Y - y coordinate
 * @param[in] W - minimal width
 * @param[in] H - minimal height
 * @return selected menu item
 */
const Fl_Menu_Item * HoverDropDown::dropDown(int X, int Y, int W, int H) {
	redraw();
	Fl_Widget_Tracker mb(this);
	const Fl_Menu_Item * m = menu()->pulldown(X, Y, W, H, 0, this);
	picked(m);
	if (m != NULL && radioCount > 0) selectRadio(m);
	if ( mb.exists() ) redraw();
	return m;
}


/**
 * Event handler.
 *
 * @param[in] e - event
 * @return 0 - if the event was not used or understood
 * @return 1 - if the event was used and can be deleted
 */
int HoverDropDown::handle(int e) {
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
		v = menu()->pulldown(x(), y(), w(), h(), menu(), this);
		if (!v || v->submenu()) return 1;
		picked(v);
		if (radioCount > 0) selectRadio(v);
		Fl::pushed(NULL);
		redraw();
		return 1;
	case FL_SHORTCUT:
		if ( Fl_Widget::test_shortcut() ) goto J1;
		v = menu()->test_shortcut();
		if ( ! v ) return 0;
		picked(v);
		if (radioCount > 0) selectRadio(v);
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
void HoverDropDown::draw() {
	/* only the menu gets drawn, hence, nothing to draw as a widget */
}


/**
 * Renders the anti-aliased unselected and selected radio glyphs into `radioOffImage` and `radioOnImage`.
 * The glyphs use a transparent background so they blend cleanly over any menu item background.
 */
void HoverDropDown::buildRadioImages() {
	/* free any previously built glyphs so the method never leaks if called more than once */
	delete radioOnImage;
	delete radioOffImage;
	radioOnImage = NULL;
	radioOffImage = NULL;
	const int sz = (FL_NORMAL_SIZE | 1); /* odd size to get a centered pixel */
	const int pad = 4;                   /* transparent gap between glyph and text */
	const int w = sz + pad;
	const int h = sz;
	const int t = 2;
	const int t2 = t * 2;
	for (int pass = 0; pass < 2; pass++) {
		const bool on = (pass != 0);
		uchar * const buf = new uchar[size_t(w) * size_t(h) * 4]();
		/* writes the anti-aliased alpha shapes into the RGBA buffer */
		const auto setColor = [=](const size_t xo, const size_t yo, const pcf::color::SplitColor & fg) {
			if (xo >= size_t(w) || yo >= size_t(h)) return;
			const float sa = 1.0f - fg[3]; /* drawCircleAA() stores 1 - coverage in the alpha channel */
			if (sa <= 0.0f) return;
			const size_t offset = (size_t(yo) * size_t(w) + size_t(xo)) * 4;
			const float da = float(buf[offset + 3]) / 255.0f;
			const float oa = sa + (da * (1.0f - sa));
			for (size_t k = 0; k < 3; k++) {
				const float dc = float(buf[offset + k]) / 255.0f;
				const float oc = (oa > 0.0f) ? (((fg[k] * sa) + (dc * da * (1.0f - sa))) / oa) : 0.0f;
				buf[offset + k] = uchar((oc * 255.0f) + 0.5f);
			}
			buf[offset + 3] = uchar((oa * 255.0f) + 0.5f);
		};
		/* filled background circle, then outline */
		pcf::image::drawEllipseAA(setColor, size_t(sz / 2), size_t(sz / 2), size_t(sz - t2), size_t(sz - t2), size_t((sz - t2) / 2), pcf::color::SplitColor(FL_BACKGROUND2_COLOR));
		pcf::image::drawEllipseAA(setColor, size_t(sz / 2), size_t(sz / 2), size_t(sz - t), size_t(sz - t), size_t(t - 1), pcf::color::SplitColor(FL_FOREGROUND_COLOR));
		if ( on ) {
			/* inner circle */
			int tW = ((sz - t2) / 2) + 1;
			if ((sz - tW) & 1) tW++;
			const int td = (sz - tW) / 2;
			pcf::image::drawEllipseAA(setColor, size_t(td + (tW / 2)), size_t(td + (tW / 2)), size_t(tW), size_t(tW), size_t((tW / 2) + 1), pcf::color::SplitColor(FL_FOREGROUND_COLOR));
		}
		Fl_RGB_Image src(buf, w, h, 4);
		Fl_RGB_Image * const img = static_cast<Fl_RGB_Image *>(src.copy()); /* owns its pixel data */
		delete [] buf;
		if ( on ) {
			radioOnImage = img;
		} else {
			radioOffImage = img;
		}
	}
}


/**
 * Sets the single selected radio item and updates the glyphs accordingly.
 *
 * @param[in] sel - newly selected item
 */
void HoverDropDown::selectRadio(const Fl_Menu_Item * sel) {
	for (int i = 0; i < radioCount; i++) {
		const bool on = (radioItems[i] == sel);
		if ( on ) {
			radioItems[i]->set();
		} else {
			radioItems[i]->clear();
		}
		radioLabels[i].labela = reinterpret_cast<const char *>(on ? radioOnImage : radioOffImage);
	}
}


/**
 * Replaces the standard radio indicators of the current menu with anti-aliased image glyphs.
 * The radio behavior is kept, but FLTK no longer draws the indicators itself. The first radio item
 * is selected.
 *
 * @remarks The menu must be writable (i.e. assigned via copy(), not menu()), as the radio flag is
 * cleared and the item text is repointed to an Fl_Multi_Label. After this call the item text no
 * longer holds a readable string. Query the selection via value() instead.
 * @remarks Call exactly once and do not replace the menu afterwards: the kept item pointers would
 * dangle and re-styling is suppressed by the single shot guard.
 */
void HoverDropDown::radioStyle() {
	if (menu() == NULL || radioCount > 0) return;
	Fl::set_labeltype(FL_IMAGE_VALUE_LABEL, imageValueLabelDraw, imageValueLabelMeasure);
	/* count the radio items */
	for (int i = 0; i < size(); i++) {
		const Fl_Menu_Item & item = menu()[i];
		if (item.text != NULL && item.radio() != 0) radioCount++;
	}
	if (radioCount <= 0) return;
	buildRadioImages();
	radioLabels = new Fl_Multi_Label[radioCount];
	radioItems = new Fl_Menu_Item *[radioCount];
	int n = 0;
	for (int i = 0; i < size() && n < radioCount; i++) {
		Fl_Menu_Item * const item = const_cast<Fl_Menu_Item *>(menu()) + i;
		if (item->text == NULL || item->radio() == 0) continue;
		radioItems[n] = item;
		radioLabels[n].typea = FL_IMAGE_VALUE_LABEL;
		radioLabels[n].labela = reinterpret_cast<const char *>(radioOffImage);
		radioLabels[n].typeb = FL_NORMAL_LABEL;
		radioLabels[n].labelb = item->text;
		item->flags &= ~FL_MENU_RADIO; /* let our glyphs be the only indicator */
		radioLabels[n].label(item);    /* sets item label type to _FL_MULTI_LABEL */
		n++;
	}
	selectRadio(radioItems[0]); /* select the first item */
}


} /* namespace gui */
} /* namespace pcf */
