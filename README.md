vkvm
====

**V**irtual **K**eyboard **V**ideo **M**ouse equipment.  
This project consists of a software part which runs on the host (controller) computer and a hardware
unit (periphery). The periphery is connected via HDMI and USB to the USB port at the controller. The
controller uses this connection to control the connected periphery computer. Control works similar to
the control of a system running within a virtual machine.  
The vkvm ensures a non-intrusive connection to the periphery device. Therefore, no special drivers or
software is needed at the periphery. This grants maximum compatibility and high security not found in
VPN or SSH like solutions and works also before OS start-up (e.g. remote BIOS control).

```txt
+------------+       +----------------------+      +---------------+
|            |  <--- | Video Capture Device | <--- |               |
| Controller |       +----------------------+      | Remote Device |
|            |  <--> | VKVM Periphery       | <--> |               |
+------------+       +----------------------+      +---------------+
```

Features
========

- control application support for Windows and Linux
- USB UVC capture device compatible at controller
- USB HID boot keyboard/mouse compatible at periphery
- easy setup and use via plug'n'play
- cheap, small, low latency and portable design
- supports direct video output capture of periphery or web cam based operation
- multiple hardware variants are available to meet different hardware assembly skills and usage requirements
- remote device access before OS startup (e.g. enter BitLocker password or set BIOS settings)

Usage
=====

Use an available display to USB (not the other way around) converter to capture the video output
of a periphery machine at the controller. See chapter example for some possible variants. Connect
the VKVM hardware device to controller and periphery to control the keyboard and mouse of the
periphery via the controller. The vkvm control application enables you to perform the described hardware
connection on software level.  
Here you choose the connected capture source and serial interface which connects the vkvm hardware.
Click into the video frame to start controlling the remote device. Hit the right SHIFT and right CTRL key
to return control. Alternatively, press CTRL-K without a connected capture source for blind control.
This can be useful when controlling the periphery device with a connected physical monitor.  
To improve periphery operation, multiple ways of inputting data from controller (paste text) are provided.
Each type exists due to different compatibilities, either by applications or on OS level.  
Most of the user interface is made up of icons to avoid language barriers.

Status LED of the VKVM periphery:

|LED                  |State     |Meaning
|:-------------------:|----------|------------------------------------------------
|                     |always off|No remote device connected.
|![LED0](doc/LED0.svg)|blinking  |Power from remote device but not USB connection.
|![LED1](doc/LED1.svg)|always on |Fully connected to remote device.

Documentation
=============

- [Hardware](doc/hardware.md)
- [Software Architecture](doc/architecture.md)
- [Serial Protocol](doc/protocol.md)

Building
========

[![Linux GCC Build Status](https://img.shields.io/github/actions/workflow/status/daniel-starke/vkvm/build.yml?label=Linux)](https://github.com/daniel-starke/vkvm/actions/workflows/build.yml)
[![Windows Visual Studio Build Status](https://img.shields.io/appveyor/ci/danielstarke/vkvm/main.svg?label=Windows)](https://ci.appveyor.com/project/danielstarke/vkvm)    

The following dependencies are given.  
Controller:  
- C99
- C++11
- [FLTK 1.3.4-1](https://github.com/fltk/fltk/tree/release-1.3.4-1)
- [nanosvg](https://github.com/memononen/nanosvg) (modified source included)
- OpenGL
- DirectShow (only on Windows)
- libv4l2 (only on Linux)
- libinput (only on Linux)

Periphery:  
- C18
- C++14
- [PlatformIO](https://platformio.org/)
- [STM32CubeDuino](https://github.com/daniel-starke/STM32CubeDuino)
- [HidDescCTC](https://github.com/daniel-starke/HidDescCTC)

Choose the appropriate Makefile to match your target system configuration.  
Windows with MinGW and GCC:  
```sh
make -f Makefile.mingw
```

Linux with GCC:
```sh
make -f Makefile.linux
```

This creates two applications:
- `vkvm`  
  The actual VKVM control application.
- `keyTest`  
  Development tool to create the OS key code to USB key code mapping.

To build and upload the firmware (depending on the target hardware):
```sh
pio run -e arduino -t upload
pio run -e vkm-b-controller -t upload
pio run -e vkm-b-periphery -t upload
```

Note that `VKVM_LED_PWM` for `vkm-b-periphery` can be changed in `platformio.ini` to any value
between 0 and 65535 to adjust the brightness of the status LED.

Also note that it is possible to set `upload_protocol = dfu` for `STM32CubeDuino` in `platformio.ini`
allowing first time programming via USB.

Open Points
===========

- [ ] command-line operations for instant video/serial connect and full screen switch
- [ ] automatically reconnect last serial/video device if it re-appears and was not disconnected by user
- [ ] alternative serial/video implementation using libusb/libuvc
- [ ] force aspect ratio option in aspect ratio button
- [ ] audio support
- [ ] configurable hot keys
- [ ] media key support (needs changes in the periphery firmware)
- [ ] fast store and load option for capture source settings
- [ ] option for file exchange via USB MTP (media transfer protocol)
- [ ] option for shared network access via USB ECM (Ethernet control model)

Limitation
==========

- keyboard and mouse input may not work with the Arduino firmware variant due to legacy BIOS
  incompatibilities with the CDC interface included by Arduino
- no OCR support (out of scope and can be done with existing screen reader applications)
- administrator/root permissions are required for the vkvm application 

Debugging
=========

Choose the appropriate Makefile to match your target system configuration.  
Windows with MinGW and GCC:  
```sh
make -f Makefile.mingw DEBUG=1
```

Linux with GCC:
```sh
make -f Makefile.linux DEBUG=1
```

The macro `VKVM_TRACE` can be defined to create the serial trace file `vkvm.log`
during runtime. The GAWK script `script/prot-dec.awk` is able to decode this file into
a human readable format. The macro can be enabled by passing `TRACE=1` to Gnumake.

To debug the periphery firmware run:
```sh
pio debug -e vkm-b-periphery
pio run -e vkm-b-periphery -t upload -t nobuild
pio debug  -e vkm-b-periphery --interface=gdb -- -x .pioinit
```

The HID reports on the periphery device can be traced using
Wireshark (see [description](https://wiki.wireshark.org/CaptureSetup/USB)).

In Linux it is also possible to read and analyze the HID reports
with the following command:
```sh
cat /sys/kernel/debug/<dev>/events
```

This shows how the Linux HID driver interprets the HID reports.

License
=======

See [LICENSE](LICENSE).  

Contributions
=============

No content contributions are accepted. Please file a bug report or feature request instead.  
This decision was made in consideration of the used license.
