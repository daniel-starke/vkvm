/**
 * @file HoverDropDown.hpp
 * @author Daniel Starke
 * @date 2019-11-23
 * @version 2026-06-14
 */
#ifndef __PCF_GUI_HOVERDROPDOWN_HPP__
#define __PCF_GUI_HOVERDROPDOWN_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Multi_Label.H>
#include <FL/Fl_RGB_Image.H>


namespace pcf {
namespace gui {


/**
 * Hover drop down menu. The hover style is always set.
 */
class HoverDropDown : public Fl_Menu_ {
private:
	Fl_RGB_Image * radioOnImage;  /**< anti-aliased selected radio glyph */
	Fl_RGB_Image * radioOffImage; /**< anti-aliased unselected radio glyph */
	Fl_Multi_Label * radioLabels; /**< per item image and text labels (size `radioCount`) */
	Fl_Menu_Item ** radioItems;   /**< managed radio items (size `radioCount`) */
	int radioCount;               /**< number of managed radio items */
public:
	explicit HoverDropDown();

	virtual ~HoverDropDown();

	inline int value() const { return Fl_Menu_::value(); }
	int value(const int v);
	int value(const Fl_Menu_Item * v);

	const Fl_Menu_Item * dropDown(int X, int Y, int W = 0, int H = 0);

	void radioStyle();

	virtual int handle(int e);
protected:
	virtual void draw();
private:
	void buildRadioImages();
	void selectRadio(const Fl_Menu_Item * sel);
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_HOVERDROPDOWN_HPP__ */
