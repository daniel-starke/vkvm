APPS = vkvm keyTest
COMMA = ,

vkvm_version = 1.2.3
vkvm_version_nums = $(subst .,$(COMMA),$(vkvm_version)),0
vkvm_version_date = 2024-11-04
vkvm_author = Daniel Starke

CPPMETAFLAGS = '-DVKVM_VERSION="$(vkvm_version) ($(vkvm_version_date))"' '-DVKVM_VERSION_NUMS=$(vkvm_version_nums)' '-DVKVM_AUTHOR="$(vkvm_author)"'
CPPFLAGS += $(CPPMETAFLAGS)

ifeq (1,$(TRACE))
 CPPFLAGS += -DVKVM_TRACE
endif

vkvm_obj = \
	libpcf/cvutf8 \
	libpcf/natcmps \
	libpcf/serial \
	pcf/color/SplitColor \
	pcf/image/Draw \
	pcf/image/Filter \
	pcf/image/Svg \
	pcf/gui/HoverButton \
	pcf/gui/HoverChoice \
	pcf/gui/HoverDropDown \
	pcf/gui/ScrollableValueInput \
	pcf/gui/SvgButton \
	pcf/gui/SvgData \
	pcf/gui/SvgView \
	pcf/gui/Utility \
	pcf/gui/VkvmControl \
	pcf/gui/VkvmView \
	pcf/serial/Port \
	pcf/serial/Vkvm \
	pcf/video/Capture \
	pcf/UtilityLinux \
	vkvm

vkvm_lib = \
	libfltk \
	libfltk_gl \
	$(OSLIBS)

keyTest_obj = \
	libpcf/serial \
	pcf/serial/Vkvm \
	keyTest

keyTest_lib = $(OSLIBS)


all: $(DSTDIR) $(addprefix $(DSTDIR)/,$(addsuffix $(BINEXT),$(APPS)))

.PHONY: $(DSTDIR)
$(DSTDIR):
	mkdir -p $(DSTDIR)

.PHONY: clean
clean:
	$(RM) -r $(DSTDIR)/*

ifeq (,$(strip $(WINDRES)))
$(DSTDIR)/vkvm$(BINEXT): $(addprefix $(DSTDIR)/,$(addsuffix $(OBJEXT),$(vkvm_obj)))
	$(AR) rs $(DSTDIR)/vkvm.a $+
	$(LD) $(LDFLAGS) $(LDWINAPP) -o $@ $(DSTDIR)/vkvm.a $(vkvm_lib:lib%=-l%)
else
$(DSTDIR)/vkvm$(BINEXT): $(addprefix $(DSTDIR)/,$(addsuffix $(OBJEXT),$(vkvm_obj))) | $(DSTDIR)/version$(OBJEXT)
	$(AR) rs $(DSTDIR)/vkvm.a $+
	$(LD) $(LDFLAGS) $(LDWINAPP) -o $@ $(DSTDIR)/vkvm.a $(vkvm_lib:lib%=-l%) $(DSTDIR)/version$(OBJEXT)
endif

$(DSTDIR)/keyTest$(BINEXT): $(addprefix $(DSTDIR)/,$(addsuffix $(OBJEXT),$(keyTest_obj)))
	$(AR) rs $(DSTDIR)/keyTest.a $+
	$(LD) $(LDFLAGS) -o $@ $(DSTDIR)/keyTest.a $(keyTest_lib:lib%=-l%)

$(DSTDIR)/%$(OBJEXT): $(SRCDIR)/%$(CEXT)
	mkdir -p "$(dir $@)"
	$(CC) $(CWFLAGS) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(DSTDIR)/%$(OBJEXT): $(SRCDIR)/%$(CXXEXT)
	mkdir -p "$(dir $@)"
	$(CXX) $(CWFLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c $<

$(DSTDIR)/%$(OBJEXT): $(SRCDIR)/%$(RCEXT)
	mkdir -p "$(dir $@)"
	$(CXX) -x c++ -E $(CPPMETAFLAGS) -o $@.ii $<
	$(WINDRES) $@.ii $@

$(SRCDIR)/license.hpp: LICENSE $(SCRIPTDIR)/convert-license.sh
	vkvm_author="$(vkvm_author)" $(SCRIPTDIR)/convert-license.sh LICENSE $@

# dependencies
$(DSTDIR)/vkvm$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/UsbKeys.hpp \
	$(SRCDIR)/libpcf/cvutf8.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/color/SplitColor.hpp \
	$(SRCDIR)/pcf/color/Utility.hpp \
	$(SRCDIR)/pcf/gui/HoverButton.hpp \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/gui/VkvmControl.hpp \
	$(SRCDIR)/pcf/gui/VkvmView.hpp \
	$(SRCDIR)/pcf/image/Draw.hpp \
	$(SRCDIR)/pcf/serial/Port.hpp \
	$(SRCDIR)/pcf/serial/Vkvm.hpp \
	$(SRCDIR)/pcf/video/Capture.hpp \
	$(SRCDIR)/pcf/UtilityLinux.hpp \
	$(SRCDIR)/pcf/Utility.hpp
$(DSTDIR)/keyTest$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/UsbKeys.hpp \
	$(SRCDIR)/libpcf/serial.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/serial/Vkvm.hpp
$(DSTDIR)/libpcf/cvutf8$(OBJEXT): \
	$(SRCDIR)/libpcf/cvutf8.h
$(DSTDIR)/libpcf/natcmps$(OBJEXT): \
	$(SRCDIR)/libpcf/natcmp.i \
	$(SRCDIR)/libpcf/natcmps.h
$(DSTDIR)/libpcf/serial$(OBJEXT): \
	$(SRCDIR)/libpcf/serial.h
$(DSTDIR)/pcf/color/SplitColor$(OBJEXT): \
	$(SRCDIR)/pcf/color/SplitColor.hpp
$(DSTDIR)/pcf/gui/DynWidthButton$(OBJEXT): \
	$(SRCDIR)/pcf/gui/DynWidthButton.hpp \
	$(SRCDIR)/pcf/gui/HoverButton.hpp
$(DSTDIR)/pcf/gui/HoverButton$(OBJEXT): \
	$(SRCDIR)/pcf/gui/HoverButton.hpp
$(DSTDIR)/pcf/gui/HoverChoice$(OBJEXT): \
	$(SRCDIR)/pcf/gui/HoverChoice.hpp
$(DSTDIR)/pcf/gui/HoverDropDown$(OBJEXT): \
	$(SRCDIR)/pcf/gui/HoverDropDown.hpp
$(DSTDIR)/pcf/gui/ScrollableValueInput$(OBJEXT): \
	$(SRCDIR)/pcf/gui/ScrollableValueInput.hpp
$(DSTDIR)/pcf/gui/SvgButton$(OBJEXT): \
	$(SRCDIR)/extern/nanosvg.h \
	$(SRCDIR)/extern/nanosvgrast.h \
	$(SRCDIR)/pcf/gui/SvgButton.hpp \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/image/Filter.hpp \
	$(SRCDIR)/pcf/image/Svg.hpp
$(DSTDIR)/pcf/gui/SvgData$(OBJEXT): \
	$(SRCDIR)/pcf/gui/SvgData.hpp
$(DSTDIR)/pcf/gui/SvgView$(OBJEXT): \
	$(SRCDIR)/pcf/gui/SvgView.hpp
$(DSTDIR)/pcf/gui/Utility$(OBJEXT): \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/Utility.hpp
$(DSTDIR)/pcf/gui/VkvmControl$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/UsbKeys.hpp \
	$(SRCDIR)/extern/nanosvg.h \
	$(SRCDIR)/extern/nanosvgrast.h \
	$(SRCDIR)/libpcf/cvutf8.h \
	$(SRCDIR)/libpcf/natcmps.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/color/Utility.hpp \
	$(SRCDIR)/pcf/gui/HoverButton.hpp \
	$(SRCDIR)/pcf/gui/HoverChoice.hpp \
	$(SRCDIR)/pcf/gui/HoverDropDown.hpp \
	$(SRCDIR)/pcf/gui/SvgButton.hpp \
	$(SRCDIR)/pcf/gui/SvgData.hpp \
	$(SRCDIR)/pcf/gui/SvgView.hpp \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/gui/VkvmControl.hpp \
	$(SRCDIR)/pcf/gui/VkvmView.hpp \
	$(SRCDIR)/pcf/image/Filter.hpp \
	$(SRCDIR)/pcf/image/Svg.hpp \
	$(SRCDIR)/pcf/serial/Port.hpp \
	$(SRCDIR)/pcf/serial/Vkvm.hpp \
	$(SRCDIR)/pcf/video/Capture.hpp \
	$(SRCDIR)/pcf/Cloneable.hpp \
	$(SRCDIR)/pcf/Utility.hpp \
	$(SRCDIR)/license.hpp
$(DSTDIR)/pcf/gui/VkvmView$(OBJEXT): \
	$(SRCDIR)/pcf/color/Utility.hpp \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/gui/VkvmView.hpp \
	$(SRCDIR)/pcf/video/Capture.hpp \
	$(SRCDIR)/pcf/Cloneable.hpp \
	$(SRCDIR)/pcf/Utility.hpp
$(DSTDIR)/pcf/image/Draw$(OBJEXT): \
	$(SRCDIR)/pcf/color/SplitColor.hpp \
	$(SRCDIR)/pcf/image/Draw.hpp
$(DSTDIR)/pcf/image/Filter$(OBJEXT): \
	$(SRCDIR)/pcf/color/SplitColor.hpp \
	$(SRCDIR)/pcf/image/Filter.hpp
$(DSTDIR)/pcf/image/Svg$(OBJEXT): \
	$(SRCDIR)/extern/nanosvg.h \
	$(SRCDIR)/extern/nanosvgrast.h \
	$(SRCDIR)/pcf/image/Svg.hpp
$(DSTDIR)/pcf/serial/Port$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/Meta.hpp \
	$(SRCDIR)/libpcf/cvutf8.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/serial/Port.hpp \
	$(SRCDIR)/pcf/serial/PortLinux.ipp \
	$(SRCDIR)/pcf/serial/PortWin.ipp \
	$(SRCDIR)/pcf/ScopeExit.hpp \
	$(SRCDIR)/pcf/UtilityLinux.hpp
$(DSTDIR)/pcf/serial/Vkvm$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/UsbKeys.hpp \
	$(SRCDIR)/libpcf/serial.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/serial/Vkvm.hpp \
	$(SRCDIR)/pcf/ScopeExit.hpp
$(DSTDIR)/pcf/video/Capture$(OBJEXT): \
	$(SRCDIR)/vkm-periphery/Framing.hpp \
	$(SRCDIR)/vkm-periphery/Protocol.hpp \
	$(SRCDIR)/libpcf/cvutf8.h \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/color/Utility.hpp \
	$(SRCDIR)/pcf/gui/HoverChoice.hpp \
	$(SRCDIR)/pcf/gui/ScrollableValueInput.hpp \
	$(SRCDIR)/pcf/gui/Utility.hpp \
	$(SRCDIR)/pcf/video/Capture.hpp \
	$(SRCDIR)/pcf/video/CaptureDirectShow.ipp \
	$(SRCDIR)/pcf/video/CaptureVideo4Linux2.ipp \
	$(SRCDIR)/pcf/Cloneable.hpp \
	$(SRCDIR)/pcf/ScopeExit.hpp \
	$(SRCDIR)/pcf/Utility.hpp \
	$(SRCDIR)/pcf/UtilityLinux.hpp \
	$(SRCDIR)/pcf/UtilityWindows.hpp
$(DSTDIR)/pcf/UtilityLinux$(OBJEXT): \
	$(SRCDIR)/libpcf/target.h \
	$(SRCDIR)/pcf/UtilityLinux.hpp \
	$(SRCDIR)/pcf/Utility.hpp
