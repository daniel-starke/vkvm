/**
 * @file VkvmControl.hpp
 * @author Daniel Starke
 * @date 2019-10-06
 * @version 2023-10-04
 */
#ifndef __PCF_GUI_VKVMCONTROL_HPP__
#define __PCF_GUI_VKVMCONTROL_HPP__

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Group.H>
#include <pcf/gui/HoverChoice.hpp>
#include <pcf/gui/HoverDropDown.hpp>
#include <pcf/gui/SvgButton.hpp>
#include <pcf/gui/SvgView.hpp>
#include <pcf/gui/Utility.hpp>
#include <pcf/gui/VkvmView.hpp>
#include <pcf/serial/Port.hpp>
#include <pcf/serial/Vkvm.hpp>


namespace pcf {
namespace gui {


/* Forward declarations. */
class VkvmControlSerialSend;
class VkvmControlRotationPopup;
class VkvmControlStatusPopup;


/**
 * Window with the virtual KVM controls.
 */
class VkvmControl:
	public Fl_Double_Window,
	public pcf::video::CaptureDeviceChangeCallback,
	public pcf::serial::SerialPortListChangeCallback,
	public pcf::serial::VkvmCallback
{
private:
	enum Tool {
		TOOL_VIDEO_SOURCE,
		TOOL_VIDEO_CONFIG,
		TOOL_SEND_ALT_F4,
		TOOL_SEND_CTRL_ALT_DEL,
		TOOL_COUNT, /**< number of tools */
		TOOL_UNKNOWN = TOOL_COUNT /**< number of tools */
	};

	pcf::video::NativeVideoCaptureProvider videoSource;
	pcf::video::CaptureDeviceList videoDevices;
	pcf::serial::NativeSerialPortProvider serialPortSource;
	pcf::serial::SerialPortList serialPorts;
	pcf::serial::SerialPort serialPort;
	pcf::serial::VkvmDevice serialDevice;
	VkvmControlSerialSend * serialSend;
	bool serialOn;
	bool serialChange;
	Fl_Window * licenseWin;
	Fl_Group * toolbar;
	HoverChoice * sourceList;
	SvgButton * videoConfig;
	SvgButton * aspectRatio;
	SvgButton * mirrorRight;
	SvgButton * mirrorUp;
	SvgButton * rotation;
	SvgButton * fullscreen;
	HoverChoice * serialList;
	SvgButton * sendKey;
	SvgButton * sendKeyChoice;
	HoverDropDown * sendKeyDropDown;
	SvgButton * license;
	Fl_Group * videoFrame;
	VkvmView * video;
	Fl_Box * status1;
	SvgView * statusConnection;
	SvgView * statusNumLock;
	SvgView * statusCapsLock;
	SvgView * statusScrollLock;
	VkvmControlRotationPopup * rotationPopup;
	VkvmControlStatusPopup * statusHistory;
	int addedWidth;
	int addedHeight;
	int minWidth;
	bool redirectInput;
	int lastMouseX;
	int lastMouseY;
	DisconnectReason lastReason;
	int shiftCtrl;
public:
	explicit VkvmControl(const int X, const int Y, const int W, const int H, const char * L = NULL);
	explicit VkvmControl(const int W, const int H, const char * L = NULL);

	virtual ~VkvmControl();
	virtual int handle(int e);
	virtual void resize(int x, int y, int w, int h);
private:
	void init();

	PCF_GUI_BIND(VkvmControl, onVideoSource, Fl_Window)
	PCF_GUI_BIND(VkvmControl, onVideoConfig, SvgButton)
	PCF_GUI_BIND(VkvmControl, onFixWindowSize, SvgButton)
	PCF_GUI_BIND(VkvmControl, onRotation, SvgButton)
	PCF_GUI_BIND(VkvmControl, onMirrorRight, SvgButton)
	PCF_GUI_BIND(VkvmControl, onMirrorUp, SvgButton)
	PCF_GUI_BIND(VkvmControl, onFullscreen, SvgButton)
	PCF_GUI_BIND(VkvmControl, onSerialSource, Fl_Window)
	PCF_GUI_BIND(VkvmControl, onSendKey, SvgButton)
	PCF_GUI_BIND(VkvmControl, onSendKeyChoice, SvgButton)
	PCF_GUI_BIND(VkvmControl, onPasteComplete, SvgButton)
	PCF_GUI_BIND(VkvmControl, onLicense, SvgButton)
	PCF_GUI_BIND(VkvmControl, onVideoResize, VkvmView)
	PCF_GUI_BIND(VkvmControl, onVideoClick, VkvmView)
	PCF_GUI_BIND(VkvmControl, onStatusClick, Fl_Box)
	PCF_GUI_BIND(VkvmControl, onQuit, Fl_Window)

	void onVideoSource(Fl_Window * w);
	void onVideoConfig(SvgButton * tool);
	void onFixWindowSize(SvgButton * tool);
	void onRotation(SvgButton * tool);
	void onMirrorRight(SvgButton * tool);
	void onMirrorUp(SvgButton * tool);
	void onFullscreen(SvgButton * tool);
	void onSerialSource(Fl_Window * w);
	void onSendKey(SvgButton * tool);
	void onSendKeyChoice(SvgButton * tool);
	void onPaste(const char * str, const int len);
	void onPasteComplete(SvgButton * tool);
	void onLicense(SvgButton * tool);
	void onVideoResize(VkvmView * view);
	void onVideoClick(VkvmView * view);
	void onStatusClick(Fl_Box * status);
	void onQuit(Fl_Window * w);

	virtual void onCaptureDeviceArrival(const char * device);
	virtual void onCaptureDeviceRemoval(const char * device);
	void onCaptureDeviceChange();
	void onCaptureViewChange();

	virtual void onSerialPortArrival(const char * port);
	virtual void onSerialPortRemoval(const char * port);
	void onSerialPortChange();
	void onSerialConnectionChange();
	void onKeyboardLedChange();
	virtual void onVkvmUsbState(const PeripheryResult res, const uint8_t usb);
	virtual void onVkvmKeyboardLeds(const PeripheryResult res, const uint8_t leds);
	virtual uint8_t onVkvmRemapKey(const uint8_t key, const int osKey, const RemapFor action);
	virtual void onVkvmConnected();
	virtual void onVkvmDisconnected(const DisconnectReason reason);

	void setRotation(const VkvmView::Rotation val);
	bool setStatusLine(const char * text = NULL, const bool copy = false);

	void connectPeriphery();
	void disconnectPeriphery();
	void startInputCapture();
	void stopInputCapture();
};


} /* namespace gui */
} /* namespace pcf */


#endif /* __PCF_GUI_VKVMCONTROL_HPP__ */
