Software Architecture
=====================

Controller
----------

### Basic Data Flow

```txt
+-------------+         +------+ 
|             | <-----> | User | ----.
|             |         +------+     |
|             |           A          V 
|             |           |    +-----------+      +--------------+      +---------+
|             | <---------+--> |VkvmDevice | <--> | SerialCommon | <--> | tSerial |
|             |           |    +-----------+      +--------------+      +---------+
| VkvmControl |           |
|             |      +----------+      +---------------+      +-----------------+
|             | <--> | VkvmView | <--- | CaptureDevice | <--- | DirectShow/V4L2 |
|             |      +----------+      +---------------+      +-----------------+
|             |
|             |      +--------------------------+
|             | <--- | NativeSerialPortProvider |
|             |      +--------------------------+
|             |      +----------------------------+
|             | <--- | NativeVideoCaptureProvider |
+-------------+      +----------------------------+
```

### GUI

The `VkvmControl` provides the main application window.

- `pcf::gui::VkvmControl` main control GUI
  - `pcf::video::NativeVideoCaptureProvider videoSource`  
    instance which provides capture devices
  - `pcf::serial::NativeSerialPortProvider serialPortSource`  
    instance which provides serial devices
  - `pcf::serial::VkvmDevice videoDevices`  
    actual video capture handle
  - `pcf::serial::SerialPort serialPorts`  
    actual serial handle

It receives device change notifications for serial and capture devices to
update the source lists and handle source device disconnections.  
The `VkvmControl` also owns sub classes for GUI control.

- `pcf::gui::<anonymous>::LicenseInfoWindow`  
  shows the license in a new window
- `pcf::gui::<anonymous>::OnClickBox`  
  helper class to add a mouse push callback to the status bar
- `pcf::gui::VkvmControlRotationPopup`  
  pop-up window for the possible capture rotation settings
- `pcf::gui::VkvmControlStatusPopup`  
  pop-up window for the status bar event history

And a control background thread to forward longer inputs (paste text) to the periphery.

- `pcf::gui::VkvmControlSerialSend`  
  forwards text in the given encoding to the periphery

The following classes implement further GUI functions used by `VkvmControl`.

- `pcf::gui::DynWidthButton`  
  add automatic width from label text function to `HoverButton`
- `pcf::gui::HoverButton`  
  flat `Fl_Button` with hover style used for toolbar icons
- `pcf::gui::HoverChoice`  
  flat `Fl_Menu_` with hover style used for toolbar menu entries
- `pcf::gui::HoverDropDown`  
  flat `Fl_Menu_` with hover style used for toolbar drop-down lists
- `pcf::gui::SvgButton`  
  button with optional hover style which shows an SVG image instead of text used for toolbar icons
- `pcf::gui::ScrollableValueInput`  
  adds value manipulation via mouse wheel to Fl_Value_Input (used in `src/pcf/video/CaptureVideo4Linux2.ipp`)
- `pcf::gui::SvgView`  
  SVG image viewer widget used for status bar icons
- `pcf::gui::VkvmView`  
  video capture device viewer

The SVG images used in this application can be found in `src/pcf/gui/SvgData.hpp`
and as original file in `src/*.svg`.

### Serial Device

These classes provide functions to list and handle serial devices for VKVM.

- `pcf::serial::SerialPort`  
  single serial port description
- `pcf::serial::SerialPortList`  
  list of serial port descriptions
- `pcf::serial::SerialPortListChangeCallback`  
  interface to receive serial port change notifications (plug/unplug)
- `pcf::serial::NativeSerialPortProvider`  
  platform specific serial port description provider (e.g. list available ports)
- `pcf::serial::VkvmCallback`  
  interface to receive VKVM command responses
- `pcf::serial::VkvmDevice`  
  handles a VKVM periphery device which all available commands; also allows to grab and forward the keyboard

The actual implementation of `VkvmDevice` and `NativeSerialPortProvider` is hidden
via PIMPLE pattern to allow easy replacement when linked dynamically.  
`VkvmDevice` and `VkvmCallback` can be used to implement a custom interface
(e.g. for scripting) to the VKVM device. `NativeSerialPortProvider` acts as helper
for this task.  
The actual OS portable serial device implementation is done in `src/libpcf/serial.h`
and `src/libpcf/serial.c`.  

### Video Device

These classes provide functions to list and handle video capture devices for VKVM.

- `pcf::serial::CaptureCallback`  
  interface to receive VKVM capture frames
- `pcf::serial::CaptureDeviceChangeCallback`  
  interface to receive video capture device change notifications (plug/unplug)
- `pcf::serial::CaptureDevice`  
  handles a VKVM video capture device
- `pcf::serial::CaptureDeviceList`  
  list of multiple capture device instances
- `pcf::serial::CaptureDeviceProvider`  
  interface to handle video capture device lists
- `pcf::serial::NativeVideoCaptureProvider`  
  platform specific implementation of `CaptureDeviceProvider`

`NativeVideoCaptureProvider` uses DirectShow on Windows and Video4Linux2 on Linux.  
The DirectShow implementations uses the native configuration dialog for the capture device.  
The Video4Linux2 implementation uses a custom FLTK dialog to configure the capture device.

### Utility

#### Color utility classes

- `pcf::color::Rgb555`  
  helper for RBG555 color format
- `pcf::color::Rgb565`  
  helper for RBG565 color format
- `pcf::color::Rgb24`  
  helper for RGB24 color format
- `pcf::color::Bgr24`  
  helper for BGR24 color format
- `pcf::color::Rgb32`  
  helper for RGB32 color format
- `pcf::color::Bgr32`  
  helper for BGR32 color format
- `pcf::color::SplitColor`  
  helper to hold and convert between RGB and HSV color values

#### Image manipulation classes/functions

- `pcf::image::blendOver()`  
  blend foreground color over the opaque background color
- `pcf::image::drawCircleAA()`  
  draw anti-aliased circle
- `pcf::image::drawEllipseAA()`  
  draw anti-aliased ellipse
- `pcf::image::Filter`
  helper class to render an RGBA32 image with different visual filters
- `pcf::image::SvgRenderer`  
  SVG renderer based on [nanosvg](https://github.com/memononen/nanosvg)

#### GUI classes/functions/macros

- `pcf::gui::noSymLabelDraw()`  
  FLTK drawing function to draw text without symbol interpretion
- `pcf::gui::noSymLabelMeasure()`  
  FLTK measurement function to draw text without symbol interpretion
- `pcf::gui::adjDpiH()`  
  returns pixel width equivalent for 96 DPI
- `pcf::gui::adjDpiV()`  
  returns pixel height equivalent for 96 DPI
- `pcf::gui::LinkedHoverState`  
  interface used to link the hover state of multiple widgets (implemented in `SvgButton`)
- `PCF_GUI_BIND()`
  creates a static member function which maps the FLTK callback to a class method
- `PCF_GUI_CALLBACK()`
  derives the static member function name created via `PCF_GUI_BIND()`

#### String functions

- `cvutf8_toUtf16()`  
  converts a null-terminated UTF-8 string to UTF-16
- `cvutf8_toUtf16N()`  
  converts a fixed length UTF-8 string to UTF-16
- `cvutf8_fromUtf16()`  
  converts a null-terminated UTF-16 string to UTF-8
- `cvutf8_fromUtf16N()`  
  converts a fixed length UTF-16 string to UTF-8
- `ncs_cmp()`  
  compares two null-terminated string with natural order
- `ncs_icmp()`  
  compares two null-terminated string with natural order case-insensitive

#### Miscellaneous classes/functions/macros

- `pcf::CloneableInterface<Base>`  
  interface for classes that implement the cloneable API
- `pcf::Cloneable`  
  implements the `CloneableInterface<Derived, Base>`
- `pcf::ScopeExit`  
  helper class to implement `makeScopeExit()`
- `pcf::makeScopeExit()`  
  executes the given function at end of scope
- `pcf::xEINTR()`  
  Linux specific helper to redo an ioctl on `EINTR` error
- `PCF_IS_WIN`  
  macro which is `1` for Windows platforms and not defined otherwise
- `PCF_IS_NO_WIN`  
  macro which is `1` for non-Windows platforms and not defined otherwise
- `PCF_IS_LINUX`  
  macro which is `1` for Linux platforms and not defined otherwise
- `PCF_IS_NO_LINUX`  
  macro which is `1` for non-Linux platforms and not defined otherwise
- `PCF_PATH_SEP`  
  macro which evaluates to the platform typical path separator
- `PCF_UNUSED()`  
  macro to suppress variable not used warnings
- `PCF_PRINTF()`  
  macro to tell the compiler to warn if incorrect arguments have been passed
  to a `printf()`-like function

Firmware
--------

@todo