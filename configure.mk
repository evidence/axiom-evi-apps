
#
# management of configuration
#

# pretty printing

_ccred:="\033[0;31m"
ccred:=$(shell echo -e $(_ccred))
ccgreen:=$(shell echo -e "\033[0;32m")
ccblue:=$(shell echo -e "\033[0;34m")
_ccend:="\033[0m"
ccend:=$(shell echo -e $(_ccend))

# predefined/old configuration
CONFMKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
-include $(CONFMKFILE_DIR)/configure-params.mk
export MODE
export DISABLE_INSTR
export DFLAGS
export ROOTFSTMP
export PREFIX
export DESTDIR

# test for configuration change

ifdef _MODE
ifdef MODE
ifneq ($(MODE),$(_MODE))
$(error You have redefined MODE. To do this you must do a "make mrproper" followed by a "make MODE=???? configure" with the new parameters)
endif
endif
MODE:=$(_MODE)
endif

ifdef _DISABLE_INSTR
ifdef DISABLE_INSTR
ifneq ($(DISABLE_INSTR),$(_DISABLE_INSTR))
$(error You have redefined DISABLE_INSTR. To do this you must do a "make mrproper" followed by a "make DISABLE_INSTR=???? configure" with the new parameters)
endif
endif
DISABLE_INSTR:=$(_DISABLE_INSTR)
endif

ifdef _KVERSION
ifdef KVERSION
ifneq ($(KVERSION),$(_KVERSION))
$(error You have redefined KVERSION. To do this you must do a "make mrproper" followed by a "make KVERSION=???? configure" with the new parameters)
endif
endif
KVERSION=$(_KVERSION)
endif

ifdef _DFLAGS
ifdef DFLAGS
ifneq ($(DFLAGS),$(_DFLAGS))
$(info You have redefined DFLAGS.)
endif
endif
DFLAGS:=$(_DFLAGS)
endif

ifdef _ROOTFSTMP
ifdef ROOTFSTMP
ifneq ($(ROOTFSTMP),$(_ROOTFSTMP))
$(info You have redefined ROOTFSTMP.)
endif
endif
ROOTFSTMP:=$(_ROOTFSTMP)
endif

ifdef _PREFIX
ifdef PREFIX
ifneq ($(PREFIX),$(_PREFIX))
$(info You have redefined PREFIX.)
endif
endif
PREFIX:=$(_PREFIX)
endif

ifdef _DESTDIR
ifdef DESTDIR
ifneq ($(DESTDIR),$(_DESTDIR))
$(info You have redefined DESTDIR.)
endif
endif
DESTDIR:=$(_DESTDIR)
endif

# test configuration definition

define ENV_HELP =
$(ccred)Command line enviroment variables not defined!$(ccend)
You must define some variable on the command line (for example "make MODE=aarch64"):
* MODE ($(ccgreen)required$(ccend))
    aarch64: use linaro to cross-compile for arm 64bit
    x86: use native x86 kernel on rootfs
* DISABLE_INSTR ($(ccgreen)optional$(ccend), used during libraries compilation)
    0: compile EXTRAE and nic user library with instrumentation support (default)
    1: disable EXTRAE compilation and nic user library instrumentation
* KVERSION ($(ccgreen)optional$(ccend), used to compile X86 in a choroot)
    you must defined this if you are compiling for X86 from a chroot and must be
    set to the kernel version (otherwise a "uname -r" is used using the chroot host
    kernel version)
* DFLAGS ($(ccgreen)optional$(ccend), other gcc parameters to add during compilation
Note that:
MODE and DISABLE_INSTR must defined during "make configure" and never changed.
KVERSION should be defined only for X86 chroot compilation during "make configure".
)
endef

ifndef MODE
$(error $(ENV_HELP))
endif

# safe parameters defaults

ifndef MODE
MODE:=aarch64
endif
ifndef DISABLE_INSTR
DISABLE_INSTR:=1
endif
ifeq ($(MODE),x86)
ifndef KVERSION
KVERSION:=$(uname -r)
endif
endif
ifndef DFLAGS
DFLAGS:=-g -O3
#DFLAGS:=-g -PPDEBUG
endif
ifndef ROOTFSTMP
ifeq ($(MODE),x86)
ROOTFSTMP:=/
else
ROOTFSTMP:=$(AXIOM_DIR)/rootfs-tmp
endif
endif
ifndef DESTDIR
DESTDIR:=$(ROOTFSTMP)
endif
ifndef PREFIX
ifeq ($(MODE),x86)
PREFIX:=/usr/local
else
PREFIX=/usr
endif
endif

# check parameters values

ifneq ($(MODE),aarch64)
ifneq ($(MODE),x86)
$(error MODE must be 'aarch64', or 'x86')
endif
endif
ifneq ($(DISABLE_INSTR),0)
ifneq ($(DISABLE_INSTR),1)
$(error DISABLE_INSTR must be '0' or '1')
endif
endif

# check environment variables for using  aarch64 mode

ifeq ($(MODE),aarch64)
ifeq ($(PETALINUX),)
$(error $(ccred)You must set the environment to work with MODE=aarch64 (see settings.sh)$(ccend))
endif
ifeq ($(LINARO),)
$(error $(ccred)You must set the environment to work with MODE=aarch64 (see settings.sh)$(ccend))
endif
endif

# show configurated variables

ifndef NCONF
$(info Actual configuration:)
ifeq ($(MODE),aarch64)
$(info $(ccgreen)MODE$(ccend)=aarch64 (using arm 64bit))
else ifeq ($(MODE),x86)
$(info $(ccgreen)MODE$(ccend)=x86 (native x86))
else
$(error $(ccred)MODE is bad$(ccend))
endif
ifeq ($(DISABLE_INSTR),0)
$(info $(ccgreen)DISABLE_INSTR$(ccend)=0 (using EXTRAE instrumentation))
else
$(info $(ccgreen)DISABLE_INSTR$(ccend)=1 (disable EXTRAE compilation and integration))
endif
ifeq ($(MODE),x86)
$(info $(ccgreen)KVERSION$(ccend)=$(KVERSION) (kernel version for X86))
endif
$(info $(ccgreen)DFLAGS$(ccend)=$(DFLAGS) (other cflags for compilation))
endif

# targets

help::
	@echo ""
	@echo "Makefile targets"
	@echo ""
	@echo "CONFIGURE"
	@echo "  configure       Configure the project"
	@echo "  "
	@echo "  configure-save  Save current cmdline variabled into configure-params.mk"
	@echo "  configure-clean Remove configure-params.mk"
	@echo " "

configure-save:
	@echo "export _MODE" >configure-params.mk
	@echo "export _DISABLE_INSTR" >>configure-params.mk
	@echo "export _DFLAGS" >>configure-params.mk
	@echo "export _ROOTFSTMP" >>configure-params.mk
	@echo "export _DESTDIR" >>configure-params.mk
	@echo "export _PREFIX" >>configure-params.mk
	@echo "_MODE:=$(MODE)" >>configure-params.mk
	@echo "_DISABLE_INSTR:=$(DISABLE_INSTR)" >>configure-params.mk
	@echo "_DFLAGS:=$(DFLAGS)" >>configure-params.mk
	@echo "_ROOTFSTMP:=$(ROOTFSTMP)" >>configure-params.mk
	@echo "_DESTDIR:=$(DESTDIR)" >>configure-params.mk
	@echo "_PREFIX:=$(PREFIX)" >>configure-params.mk
ifdef KVERSION
	@echo "export _KVERSION" >>configure-params.mk
	@echo "_KVERSION:=$(KVERSION)" >>configure-params.mk
endif

configure-clean:
	-rm -f configure-params.mk
