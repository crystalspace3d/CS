#
# installation makefile
#
#
# Places you can add for installation:
#
# TO_INSTALL.ROOT : files will be added in the $(INSTALL_DIR)/ dir itself.
# TO_INSTALL.EXE  : files will be put in bin/
# TO_INSTALL.DATA : files will be put in data/
# TO_INSTALL.CONFIG: files will be put in data/config/
# TO_INSTALL.STATIC_LIBS: files will be put in lib/
# TO_INSTALL.DYNAMIC_LIBS: files could be put in lib/, but could also end
#                     up in a platform specific location (e.g. System folder).
# TO_INSTALL.INCLUDE does not exist, the entire include/ hierarchy is copied
#                   to include/. (max 3 levels deep now)

.PHONY: install_config install_data install_dynamiclibs install_staticlibs \
  install_exe install_include install_root install_logfile

INSTALL_LOG=$(INSTALL_DIR)/install.log

# the $(INSTALL_DIR)/include dir is done later
$(INSTALL_DIR) $(INSTALL_DIR)/data $(INSTALL_DIR)/bin $(INSTALL_DIR)/lib \
$(INSTALL_DIR)/data/config:
	$(MKDIR)

# list the install.log in the install.log to be deleted.
install_logfile:
	@echo $(INSTALL_LOG) >> $(INSTALL_LOG)

install_config: $(INSTALL_DIR)/data/config
	$(CP) $(TO_INSTALL.CONFIG) $(INSTALL_DIR)/data/config
	@echo $(addprefix $(INSTALL_DIR)/data/config/, \
	  $(notdir $(TO_INSTALL.CONFIG))) >> $(INSTALL_LOG)

install_data: $(INSTALL_DIR)/data
	$(CP) $(TO_INSTALL.DATA) $(INSTALL_DIR)/data
	@echo $(addprefix $(INSTALL_DIR)/data/, \
	  $(notdir $(TO_INSTALL.DATA))) >> $(INSTALL_LOG)

install_dynamiclibs: $(INSTALL_DIR)/lib
	$(CP) $(TO_INSTALL.DYNAMIC_LIBS) $(INSTALL_DIR)/lib
	@echo $(addprefix $(INSTALL_DIR)/lib/, \
	  $(notdir $(TO_INSTALL.DYNAMIC_LIBS))) >> $(INSTALL_LOG)

install_staticlibs: $(INSTALL_DIR)/lib
	$(CP) $(TO_INSTALL.STATIC_LIBS) $(INSTALL_DIR)/lib
	@echo $(addprefix $(INSTALL_DIR)/lib/, \
	  $(notdir $(TO_INSTALL.STATIC_LIBS))) >> $(INSTALL_LOG)

install_exe: $(INSTALL_DIR)/bin
	$(CP) $(TO_INSTALL.EXE) $(INSTALL_DIR)/bin
	@echo $(addprefix $(INSTALL_DIR)/bin/, \
	  $(notdir $(TO_INSTALL.EXE))) >> $(INSTALL_LOG)

install_root: $(INSTALL_DIR)
	$(CP) $(TO_INSTALL.ROOT) $(INSTALL_DIR)
	@echo $(addprefix $(INSTALL_DIR)/, \
	  $(notdir $(TO_INSTALL.ROOT))) >> $(INSTALL_LOG)

# Copying the include/ hierarchy, only the header files detected here
# will be copied
INSTALL_INCLUDE.FILES = $(wildcard include/*h include/*/*h include/*/*/*h)

# take all .h files in include/, take their directory parts, sort those,
# and remove trailing /, then add the INSTALLDIR prefix
INSTALL_INCLUDE.DIR = $(addprefix $(INSTALL_DIR)/,  \
  $(patsubst %/, %,$(sort $(dir $(INSTALL_INCLUDE.FILES)))))
INSTALL_INCLUDE.DESTFILES = $(addprefix $(INSTALL_DIR)/,  \
  $(patsubst %/, %,$(sort $(INSTALL_INCLUDE.FILES))))

$(INSTALL_INCLUDE.DIR):
	$(MKDIR)

$(INSTALL_INCLUDE.DESTFILES): $(INSTALL_DIR)/% : %
	$(CP) $< $@
	@echo $@ >> $(INSTALL_LOG)

install_include: $(INSTALL_DIR)/include $(INSTALL_INCLUDE.DIR) \
  $(INSTALL_INCLUDE.DESTFILES)


