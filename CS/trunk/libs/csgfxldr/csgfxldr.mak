#------------------------------------------------------------------------------#
#                  Submakefile for Crystal Space utility library               #
#------------------------------------------------------------------------------#

# Library description
DESCRIPTION.csgfxldr = Crystal Space graphics image file loader

#-------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Library-specific help commands
LIBHELP += $(NEWLINE)echo $"  make csgfxldr     Make the $(DESCRIPTION.csgfxldr)$"

ifeq ($(DO_GIF),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_GIF$">>$@
endif
ifeq ($(DO_BMP),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_BMP$">>$@
endif
ifeq ($(DO_TGA),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_TGA$">>$@
endif
ifeq ($(DO_PNG),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_PNG$">>$@
endif
ifeq ($(DO_JPG),yes)
  MAKE_VOLATILE_H += $(NEWLINE)echo $"\#define DO_JPG$">>$@
endif

endif # ifeq ($(MAKESECTION),rootdefines)

#-------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: csgfxldr

all libs: csgfxldr
csgfxldr:
	$(MAKE_TARGET)

endif # ifeq ($(MAKESECTION),roottargets)

#-------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp libs/csgfxldr

SRC.CSGFXLDR = libs/csgfxldr/csimage.cpp libs/csgfxldr/iimage.cpp \
  libs/csgfxldr/pcx.cpp

ifeq ($(DO_GIF),yes)
  SRC.CSGFXLDR+=libs/csgfxldr/gifimage.cpp
endif
ifeq ($(DO_BMP),yes)
  SRC.CSGFXLDR+=libs/csgfxldr/bmpimage.cpp
endif
ifeq ($(DO_TGA),yes)
  SRC.CSGFXLDR+=libs/csgfxldr/tgaimage.cpp
endif
ifeq ($(DO_PNG),yes)
  SRC.CSGFXLDR+=libs/csgfxldr/pngimage.cpp
  LIBS.EXE+=$(PNG_LIBS)
endif
ifeq ($(DO_JPG),yes)
  SRC.CSGFXLDR+=libs/csgfxldr/jpgimage.cpp
  LIBS.EXE+=$(JPG_LIBS)
endif

CSGFXLDR.LIB = $(OUT)$(LIB_PREFIX)csgfxldr$(LIB)
OBJ.CSGFXLDR = $(addprefix $(OUT),$(notdir $(SRC.CSGFXLDR:.cpp=$O)))

endif # ifeq ($(MAKESECTION),postdefines)

#------------------------------------------------------------------ targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: csgfxldr csgfxldrclean

all: $(CSGFXLDR.LIB)
csgfxldr: $(OUTDIRS) $(CSGFXLDR.LIB)
clean: csgfxldrclean

$(CSGFXLDR.LIB): $(OBJ.CSGFXLDR)
	$(DO.STATIC.LIBRARY)

csgfxldrclean:
	-$(RM) $(CSGFXLDR.LIB)

ifdef DO_DEPEND
$(OUTOS)csgfxldr.dep: $(SRC.CSGFXLDR)
	$(DO.DEP)
endif

-include $(OUTOS)csgfxldr.dep

endif # ifeq ($(MAKESECTION),targets)
