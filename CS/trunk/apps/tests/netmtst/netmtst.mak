DESCRIPTION.netmantest = Crystal Space network manager test

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

# Application
APPHELP += \
  $(NEWLINE)@echo $"  make netmantest   Make the $(DESCRIPTION.netmantest)$"

endif # ifeq ($(MAKESECTION),rootdefines)

#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: netmantest netmantestclean

all apps: netmantest

netmantest:
	$(MAKE_APP)
netmantestclean:
	$(MAKE_CLEAN)

endif # ifeq ($(MAKESECTION),roottargets)

#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

NETMANTEST.EXE = netmtst$(EXE)
DIR.NETMANTEST = apps/tests/netmtst
OUT.NETMANTEST = $(OUT)/$(DIR.NETMANTEST)
INC.NETMANTEST = $(wildcard $(DIR.NETMANTEST)/*.h)
SRC.NETMANTEST = $(wildcard $(DIR.NETMANTEST)/*.cpp)
OBJ.NETMANTEST = \
  $(addprefix $(OUT.NETMANTEST)/,$(notdir $(SRC.NETMANTEST:.cpp=$O)))
DEP.NETMANTEST = CSTOOL CSUTIL CSSYS CSGEOM CSGFX
LIB.NETMANTEST = $(foreach d,$(DEP.NETMANTEST),$($d.LIB))

OUTDIRS += $(OUT.NETMANTEST)

MSVC.DSP += NETMANTEST
DSP.NETMANTEST.NAME = netmtst
DSP.NETMANTEST.TYPE = appcon

endif # ifeq ($(MAKESECTION),postdefines)

#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: build.netmantest netmantestclean netmantestcleandep

build.netmantest: $(OUTDIRS) $(NETMANTEST.EXE)
clean: netmantestclean

$(OUT.NETMANTEST)/%$O: $(DIR.NETMANTEST)/%.cpp
	$(DO.COMPILE.CPP)

$(NETMANTEST.EXE): $(OBJ.NETMANTEST) $(LIB.NETMANTEST)
	$(DO.LINK.EXE)

netmantestclean:
	-$(RM) netmantest.txt
	-$(RMDIR) $(NETMANTEST.EXE) $(OBJ.NETMANTEST)

cleandep: netmantestcleandep
netmantestcleandep:
	-$(RM) $(OUT.NETMANTEST)/netmtst.dep

ifdef DO_DEPEND
dep: $(OUT.NETMANTEST) $(OUT.NETMANTEST)/netmtst.dep
$(OUT.NETMANTEST)/netmtst.dep: $(SRC.NETMANTEST)
	$(DO.DEPEND)
else
-include $(OUT.NETMANTEST)/netmtst.dep
endif

endif # ifeq ($(MAKESECTION),targets)
