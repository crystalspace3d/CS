# This is a subinclude file used to define the rules needed
# to build the Windows DirectDraw 2D driver

# Driver description
DESCRIPTION.ddraw = Crystal Space DirectDraw 2D driver

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Driver-specific help commands
DRIVERHELP += \
  $(NEWLINE)echo $"  make ddraw        Make the $(DESCRIPTION.ddraw)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: ddraw ddrawclean
all softcanvas plugins drivers drivers2d: ddraw

ddraw:
	$(MAKE_TARGET) MAKE_DLL=yes
ddrawclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp plugins/video/canvas/ddraw plugins/video/canvas/common \
  plugins/video/canvas/directxcommon

ifeq ($(USE_PLUGINS),yes)
  DDRAW = $(OUTDLL)csddraw$(DLL)
  LIB.DDRAW = $(foreach d,$(DEP.DDRAW),$($d.LIB))
  LIB.DDRAW.SPECIAL = $(LFLAGS.l)ddraw
  TO_INSTALL.DYNAMIC_LIBS += $(DDRAW)
else
  DDRAW = $(OUT)$(LIB_PREFIX)csddraw$(LIB)
  DEP.EXE += $(DDRAW)
  LIBS.EXE += $(LFLAGS.l)ddraw
  SCF.STATIC += csddraw
  TO_INSTALL.STATIC_LIBS += $(DDRAW)
endif

INC.DDRAW = $(wildcard plugins/video/canvas/ddraw/*.h \
  $(wildcard plugins/video/canvas/directxcommon/*.h $(INC.COMMON.DRV2D)))
SRC.DDRAW = $(wildcard plugins/video/canvas/ddraw/*.cpp \
  $(wildcard plugins/video/canvas/directxcommon/*.cpp $(SRC.COMMON.DRV2D)))
OBJ.DDRAW = $(addprefix $(OUT),$(notdir $(SRC.DDRAW:.cpp=$O)))
DEP.DDRAW = CSUTIL CSSYS CSUTIL

MSVC.DSP += DDRAW
DSP.DDRAW.NAME = csddraw
DSP.DDRAW.TYPE = plugin
DSP.DDRAW.LIBS = ddraw

endif # ifeq ($(MAKESECTION),postdefines)

#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: ddraw ddrawclean

ddraw: $(OUTDIRS) $(DDRAW)

$(DDRAW): $(OBJ.DDRAW) $(LIB.DDRAW)
	$(DO.PLUGIN) $(LIB.DDRAW.SPECIAL)

clean: ddrawclean
ddrawclean:
	$(RM) $(DDRAW) $(OBJ.DDRAW)

ifdef DO_DEPEND
depend: $(OUTOS)ddraw.dep
$(OUTOS)ddraw.dep: $(SRC.DDRAW)
	$(DO.DEP)
else
-include $(OUTOS)ddraw.dep
endif

endif # ifeq ($(MAKESECTION),targets)
