# This is a subinclude file used to define the rules needed
# to build the 3D line rendering driver -- line

# Driver description
DESCRIPTION.line = Crystal Space line 3D driver

#-------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Driver-specific help commands
DRIVERHELP += $(NEWLINE)echo $"  make line         Make the $(DESCRIPTION.line)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#-------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: line

all plugins drivers drivers3d: line

line:
	$(MAKE_TARGET) MAKE_DLL=yes
lineclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#-------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp plugins/video/renderer/line

ifeq ($(USE_SHARED_PLUGINS),yes)
  LINE3D=$(OUTDLL)line3d$(DLL)
  DEP.LINE3D=$(CSGEOM.LIB) $(CSGFXLDR.LIB) $(CSUTIL.LIB) $(CSSYS.LIB)
else
  LINE3D=$(OUT)$(LIB_PREFIX)line$(LIB)
  DEP.EXE+=$(LINE3D)
  CFLAGS.STATIC_SCF+=$(CFLAGS.D)SCL_LINE3D
endif
DESCRIPTION.$(LINE3D) = $(DESCRIPTION.line)
SRC.LINE3D = $(wildcard plugins/video/renderer/line/*.cpp) \
  plugins/video/renderer/common/txtmgr.cpp plugins/video/renderer/common/dtmesh.cpp \
  plugins/video/renderer/common/dpmesh.cpp
OBJ.LINE3D = $(addprefix $(OUT),$(notdir $(subst .asm,$O,$(SRC.LINE3D:.cpp=$O))))

endif # ifeq ($(MAKESECTION),postdefines)

#------------------------------------------------------------------ targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: line lineclean

# Chain rules
all: $(LINE3D)
clean: lineclean

line: $(OUTDIRS) $(LINE3D)

# Extra dependencies not generated by "make depend"
$(LINE3D): $(OBJ.LINE3D) $(DEP.LINE3D)
	$(DO.PLUGIN)

lineclean:
	$(RM) $(LINE3D) $(OBJ.LINE3D)

ifdef DO_DEPEND
dep: $(OUTOS)line3d.dep
$(OUTOS)line3d.dep: $(SRC.LINE3D)
	$(DO.DEP)
else
-include $(OUTOS)line3d.dep
endif

endif # ifeq ($(MAKESECTION),targets)
