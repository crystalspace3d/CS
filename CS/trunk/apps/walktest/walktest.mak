# Application description
DESCRIPTION.walk = Crystal Space WalkTest demo executable

#-------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Application-specific help commands
APPHELP += $(NEWLINE)echo $"  make walk         Make the $(DESCRIPTION.walk)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#-------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: walk

all apps: walk
walk:
	$(MAKE_TARGET)

endif # ifeq ($(MAKESECTION),roottargets)

#-------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp apps/walktest apps/support

WALKTEST.EXE=walktest$(EXE)
SRC.WALKTEST = $(wildcard apps/walktest/*.cpp) \
  apps/support/static.cpp apps/support/command.cpp
OBJ.WALKTEST = $(addprefix $(OUT),$(notdir $(SRC.WALKTEST:.cpp=$O)))
DESCRIPTION.$(WALKTEST.EXE) = $(DESCRIPTION.walk)

endif # ifeq ($(MAKESECTION),postdefines)

#------------------------------------------------------------------ targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: walk walkclean

all: $(WALKTEST.EXE)
walk: $(OUTDIRS) $(WALKTEST.EXE)
clean: walkclean

$(WALKTEST.EXE): $(DEP.EXE) $(OBJ.WALKTEST) \
  $(CSTOOLS.LIB) $(CSPARSER.LIB) $(CSENGINE.LIB) $(CSTERR.LIB) $(CSSCRIPT.LIB) \
  $(CSGEOM.LIB) $(CSSFXLDR.LIB) $(CSGFXLDR.LIB) $(CSCOM.LIB) $(CSSYS.LIB) \
  $(CSINPUT.LIB) $(CSOBJECT.LIB) $(CSUTIL.LIB)
	$(DO.LINK.EXE)

walkclean:
	-$(RM) $(WALKTEST.EXE)

ifdef DO_DEPEND
depend: $(OUTOS)walktest.dep
$(OUTOS)walktest.dep: $(SRC.WALKTEST)
	$(DO.DEP)
else
-include $(OUTOS)walktest.dep
endif

endif # ifeq ($(MAKESECTION),targets)
