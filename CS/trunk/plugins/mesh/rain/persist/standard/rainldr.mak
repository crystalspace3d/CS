DESCRIPTION.rainldr = Rain mesh object loader

#------------------------------------------------------------- rootdefines ---#
ifeq ($(MAKESECTION),rootdefines)

PLUGINHELP += \
  $(NEWLINE)echo $"  make rainldr      Make the $(DESCRIPTION.rainldr)$"

endif # ifeq ($(MAKESECTION),rootdefines)
#------------------------------------------------------------- roottargets ---#
ifeq ($(MAKESECTION),roottargets)

.PHONY: rainldr rainldrclean
plugins meshes all: rainldr

rainldrclean:
	$(MAKE_CLEAN)
rainldr:
	$(MAKE_TARGET) MAKE_DLL=yes

endif # ifeq ($(MAKESECTION),roottargets)
#------------------------------------------------------------- postdefines ---#
ifeq ($(MAKESECTION),postdefines)

vpath %.cpp $(SRCDIR)/plugins/mesh/rain/persist/standard

ifeq ($(USE_PLUGINS),yes)
  RAINLDR = $(OUTDLL)/rainldr$(DLL)
  LIB.RAINLDR = $(foreach d,$(DEP.RAINLDR),$($d.LIB))
  TO_INSTALL.DYNAMIC_LIBS += $(RAINLDR)
else
  RAINLDR = $(OUT)/$(LIB_PREFIX)rainldr$(LIB)
  DEP.EXE += $(RAINLDR)
  SCF.STATIC += rainldr
  TO_INSTALL.STATIC_LIBS += $(RAINLDR)
endif

INF.RAINLDR = $(SRCDIR)/plugins/mesh/rain/persist/standard/rainldr.csplugin
INC.RAINLDR = $(wildcard $(addprefix $(SRCDIR)/,plugins/mesh/rain/persist/standard/*.h))
SRC.RAINLDR = $(wildcard $(addprefix $(SRCDIR)/,plugins/mesh/rain/persist/standard/*.cpp))
OBJ.RAINLDR = $(addprefix $(OUT)/,$(notdir $(SRC.RAINLDR:.cpp=$O)))
DEP.RAINLDR = CSGEOM CSUTIL CSUTIL

MSVC.DSP += RAINLDR
DSP.RAINLDR.NAME = rainldr
DSP.RAINLDR.TYPE = plugin

endif # ifeq ($(MAKESECTION),postdefines)
#----------------------------------------------------------------- targets ---#
ifeq ($(MAKESECTION),targets)

.PHONY: rainldr rainldrclean
rainldr: $(OUTDIRS) $(RAINLDR)

$(RAINLDR): $(OBJ.RAINLDR) $(LIB.RAINLDR)
	$(DO.PLUGIN)

clean: rainldrclean
rainldrclean:
	-$(RMDIR) $(RAINLDR) $(OBJ.RAINLDR) $(OUTDLL)/$(notdir $(INF.RAINLDR))

ifdef DO_DEPEND
dep: $(OUTOS)/rainldr.dep
$(OUTOS)/rainldr.dep: $(SRC.RAINLDR)
	$(DO.DEP)
else
-include $(OUTOS)/rainldr.dep
endif

endif # ifeq ($(MAKESECTION),targets)
