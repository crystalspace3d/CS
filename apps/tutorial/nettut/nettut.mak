# Application description
DESCRIPTION.nettut = Crystal Space network tutorial

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Application-specific help commands
APPHELP += \
  $(NEWLINE)echo $"  make nettut       Make the $(DESCRIPTION.nettut)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: nettut netutclean

all apps: nettut
nettut:
	$(MAKE_APP)
netutclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp apps/tutorial/nettut

NETTUT.EXE = nettut$(EXE)
INC.NETTUT = $(wildcard apps/tutorial/nettut/*.h)
SRC.NETTUT = $(wildcard apps/tutorial/nettut/*.cpp)
OBJ.NETTUT = $(addprefix $(OUT)/,$(notdir $(SRC.NETTUT:.cpp=$O)))
DEP.NETTUT = CSTOOL CSGFX CSUTIL CSSYS CSGEOM CSUTIL CSSYS
LIB.NETTUT = $(foreach d,$(DEP.NETTUT),$($d.LIB))

#TO_INSTALL.EXE += $(NETTUT.EXE)

MSVC.DSP += NETTUT
DSP.NETTUT.NAME = nettut
DSP.NETTUT.TYPE = appcon

endif # ifeq ($(MAKESECTION),postdefines)

#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: build.nettut netutclean

all: $(NETTUT.EXE)
build.nettut: $(OUTDIRS) $(NETTUT.EXE)
clean: netutclean

$(NETTUT.EXE): $(DEP.EXE) $(OBJ.NETTUT) $(LIB.NETTUT)
	$(DO.LINK.EXE)

netutclean:
	-$(RM) $(NETTUT.EXE) $(OBJ.NETTUT)

ifdef DO_DEPEND
dep: $(OUTOS)/nettut.dep
$(OUTOS)/nettut.dep: $(SRC.NETTUT)
	$(DO.DEP)
else
-include $(OUTOS)/nettut.dep
endif

endif # ifeq ($(MAKESECTION),targets)
