################################################################################
#
#           This is the root makefile for the Crystal Space engine
#
################################################################################

.PHONY: help banner showplatforms showconfig platforms all doc api depend \
  configure clean cleanlib cleandep distclean libs drivers drivers2d \
  drivers3d snddrivers netdrivers

# The following two symbols are intended to be used in "echo" commands
# config.mak can override them depending on configured system requirements
"='
|=|
-include config.mak

include mk/user.mak
include mk/common.mak

# Find all available system targets
SYSTARGETS=$(patsubst mk/system/%.mak,%,$(wildcard mk/system/*.mak))

# The initial driver and application targets help text
DRVHELP = \
  echo $"The following Crystal Space drivers can be built:$"
APPHELP = \
  echo $"The following Crystal Space applications can be built:$"
LIBHELP = \
  echo $"The following Crystal Space libraries can be built:$"
SYSHELP = \
  echo $"The makefile system can be configured for the following platforms:$"
define SYSMODIFIERSHELP
  echo $"  -*- Modifiers -*-$"
  echo $"  USE_DLL=yes$|no$"
  echo $"      Build drivers/plugins as dynamic/static modules$"
  echo $"  MODE=optimize$|debug$|profile$"
  echo $"      Select one of three available compilation modes$"
endef
# This macro is used to rebuild "volatile.h"
# You're free to add any commands you want to it in submakefiles
define MAKE_VOLATILE_H
  echo $"#define OS_$(OS)$">>volatile.tmp
  echo $"#define PROC_$(PROC)$">>volatile.tmp
  echo $"#define COMP_$(COMP)$">>volatile.tmp
endef

# If there is no target defined (makefile system were not configured),
# look which targets are available in mk/system directory.
ifeq ($(TARGET),)

MAKESECTION=confighelp
-include $(wildcard mk/system/*.mak)

help: banner showplatforms

else

MAKESECTION=rootdefines
include mk/subs.mak

help: banner showconfig drvhelp libhelp apphelp pseudohelp

depend:
	@$(MAKE) --no-print-directory -f mk/cs.mak $@ DO_DEPEND=yes

doc api clean cleanlib cleandep distclean:
	@$(MAKE) --no-print-directory -f mk/cs.mak $@

unknown:
	$(RM) config.mak
	$(RM) include/volatile.h

platforms:
	@echo $(SEPARATOR)
	@$(MAKE) --no-print-directory TARGET= showplatforms

showconfig:
	@echo $"  Configured for $(DESCRIPTION.$(TARGET)) with the following modifiers:$"
	@echo $"  USE_DLL=$(USE_DLL) MODE=$(MODE) $(SYSMODIFIERS)$"
	@echo $(SEPARATOR)

drvhelp:
	@$(DRVHELP)
	@echo $(SEPARATOR)

libhelp:
	@$(LIBHELP)
	@echo $(SEPARATOR)

apphelp:
	@$(APPHELP)
	@echo $(SEPARATOR)

pseudohelp:
	@echo $"  make apps         Make all applications$"
	@echo $"  make libs         Make all static libraries$"
	@echo $"  make drivers      Make all drivers$"
	@echo $"  make drivers2d    Make all supported 2D graphics drivers$"
	@echo $"  make drivers3d    Make all supported 3D graphics drivers (renderers)$"
	@echo $"  make snddrivers   Make all supported sound drivers$"
	@echo $"  make netdrivers   Make all supported sound drivers$"
	@echo $"  make depend       Make dependencies (recommended!)$"
	@echo $"  make doc          Make documentation using Doc++$"
	@echo $"  make api          Make API documentation using Doc++$"
	@echo $"  make clean        Clean all files generated during build$"
	@echo $"  make cleanlib     Clean all dynamic libraries$"
	@echo $"  make cleandep     Clean all dependency rule files$"
	@echo $"  make distclean    Clean everything$"
	@echo $"  make platforms    List the available target platforms$"
	@echo $(SEPARATOR)

endif

banner:
	@echo $(SEPARATOR)
	@echo $"  Before compiling Crystal Space examine mk/user.mak and see if settings$"
	@echo $"  are suited for your system. Note that you need at least one renderer and$"
	@echo $"  at least one 2D driver in order to be able to run the engine.$"
	@echo $(SEPARATOR)

showplatforms:
	@$(SYSHELP)
	@$(SYSMODIFIERSHELP)
	@echo $"  Example: make linux USE_DLL=yes MODE=debug$"
	@echo $(SEPARATOR)

# Prepare for specific system
# WARNING: Try to avoid quotes in most important "echo" statements
# since several systems (OS/2, DOS and WIN32) have a "echo" that does
# literal output, i.e. they do not strip quotes from string.
$(SYSTARGETS):
	@echo TARGET = $@>config.tmp
	@$(MAKE) --no-print-directory ROOTCONFIG=config configure TARGET=$@

ifeq ($(ROOTCONFIG),config)

# Force config.tmp to be always rebuilt
.PHONY: config.tmp

configure: config.tmp
	@$(MAKE) --no-print-directory ROOTCONFIG=volatile configure
	@echo $(SEPARATOR)
	@echo $"  Makefiles are now configured for $(DESCRIPTION.$(TARGET)).$"
	@echo $(SEPARATOR)

config.tmp:
	@echo MODE = $(MODE)>>config.tmp
	@echo USE_DLL = $(USE_DLL)>>config.tmp
	@$(SYSCONFIG)
	$(subst DEST,config.mak,$(UPD))

endif
ifeq ($(ROOTCONFIG),volatile)

include mk/volatile.mak

configure: volatile.tmp

# NOTE: Do not remove the $(subst volatile.t...) calls.  They work around a
# bug in GNU make on MacOS/X Server.  Specifically, make corrupts
# MAKE_VOLATILE_H (and probably other variables) by truncating values which
# are appended to it with +=.  In this case we see instances of volatile.t and
# volatile.tm, which is clearly incorrect.  To work around the problem, we
# manually translate .t and .tm back into .tmp.  Do not replace the $(subst...)
# calls with $(patsubst...) since patsubst is incapable of transforming the
# sort of strings which MAKE_VOLATILE_H contains.

volatile.tmp: config.mak
	@echo $"/* This file is automatically generated, do not change manually! */$">volatile.tmp
	@echo $"#ifndef __VOLATILE_H__$">>volatile.tmp
	@echo $"#define __VOLATILE_H__$">>volatile.tmp
	@$(subst volatile.t,volatile.tmp,\
	 $(subst volatile.tm,volatile.t,\
	 $(subst volatile.tmp,volatile.t,$(MAKE_VOLATILE_H))))
	@echo $"#endif // __VOLATILE_H__$">>volatile.tmp
	$(subst DEST,include/volatile.h,$(UPD))

endif

MAKESECTION=roottargets
include mk/subs.mak
