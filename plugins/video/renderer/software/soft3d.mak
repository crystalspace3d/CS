# This is a subinclude file used to define the rules needed
# to build the 3D software rendering driver -- soft

# Driver description
DESCRIPTION.soft = Crystal Space software 3D driver

#-------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Driver-specific help commands
DRIVERHELP += $(NEWLINE)echo $"  make soft         Make the $(DESCRIPTION.soft)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#-------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: soft

all plugins drivers drivers3d: soft

soft:
	$(MAKE_TARGET) MAKE_DLL=yes
softclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#-------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp plugins/video/renderer/software plugins/video/renderer/common

ifeq ($(USE_SHARED_PLUGINS),yes)
  SOFT3D=$(OUTDLL)soft3d$(DLL)
  DEP.SOFT3D=$(CSGEOM.LIB) $(CSGFXLDR.LIB) $(CSUTIL.LIB) $(CSSYS.LIB)
  TO_INSTALL.DYNAMIC_LIBS+=$(SOFT3D)
else
  SOFT3D=$(OUT)$(LIB_PREFIX)sft3d$(LIB)
  DEP.EXE+=$(SOFT3D)
  CFLAGS.STATIC_SCF+=$(CFLAGS.D)SCL_SOFT3D
  TO_INSTALL.STATIC_LIBS+=$(SOFT3D)
endif

TO_INSTALL.CONFIG += data/config/soft3d.cfg
DESCRIPTION.$(SOFT3D) = $(DESCRIPTION.soft)
SRC.SOFT3D += $(wildcard plugins/video/renderer/software/*.cpp) \
	plugins/video/renderer/common/txtmgr.cpp \
	plugins/video/renderer/common/dtmesh.cpp \
	plugins/video/renderer/common/dpmesh.cpp

ifeq ($(NASM.INSTALLED),yes)
SRC.SOFT3D += $(wildcard plugins/video/renderer/software/i386/*.asm)
endif
OBJ.SOFT3D = $(addprefix $(OUT),$(notdir $(subst .asm,$O,$(SRC.SOFT3D:.cpp=$O))))
NASMFLAGS.SOFT3D = -i./plugins/video/renderer/software/i386/

endif # ifeq ($(MAKESECTION),postdefines)

#------------------------------------------------------------------ targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: soft softclean

# Chain rules
all: $(SOFT3D)
clean: softclean

soft: $(OUTDIRS) $(SOFT3D)

# Extra dependencies not generated by "make depend"
$(OUT)scan8a$O: plugins/video/renderer/software/i386/cs.ash plugins/video/renderer/software/i386/scan.ash
$(OUT)scan16a$O: plugins/video/renderer/software/i386/cs.ash plugins/video/renderer/software/i386/scan.ash
$(OUT)scan32a$O: plugins/video/renderer/software/i386/cs.ash plugins/video/renderer/software/i386/scan.ash
$(OUT)cpuid$O: plugins/video/renderer/software/i386/cs.ash

$(OUT)%$O: plugins/video/renderer/software/i386/%.asm
	$(DO.COMPILE.ASM) $(NASMFLAGS.SOFT3D)

$(SOFT3D): $(OBJ.SOFT3D) $(DEP.SOFT3D)
	$(DO.PLUGIN)

softclean:
	$(RM) $(SOFT3D) $(OBJ.SOFT3D)

ifdef DO_DEPEND
dep: $(OUTOS)soft3d.dep
$(OUTOS)soft3d.dep: $(SRC.SOFT3D)
	$(DO.DEP)
else
-include $(OUTOS)soft3d.dep
endif

endif # ifeq ($(MAKESECTION),targets)
