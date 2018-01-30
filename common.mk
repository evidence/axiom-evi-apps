
# configure parameters

COMMKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
include $(COMMKFILE_DIR)/configure-params.mk
ifndef MODE
MODE:=$(_MODE)
endif
ifndef DISABLE_INSTR
DISABLE_INSTR=$(_DISABLE_INSTR)
endif
ifndef DFLAGS
DFLAGS:=$(_DFLAGS)
endif
ifndef ROOTFSTMP
ROOTFSTMP:=$(_ROOTFSTMP)
endif
ifndef PREFIX
PREFIX=$(_PREFIX)
endif
ifndef DESTDIR
DESTDIR=$(_DESTDIR)
endif
ifeq ($(MODE),x86)
ifndef KVERSION
KVERSION:=$(_KVERSION)
endif
endif

#
# utility
#

# number of build host cores
BUILDCPUS := $(shell getconf _NPROCESSORS_ONLN)

#
# version numbers
#

VERSION := $(shell git describe --tags --dirty | sed -re 's/^axiom-v([0-9.]*).*/\1/')
MAJOR := $(shell echo $(VERSION) | sed -e 's/\..*//')
MINOR := $(shell echo $(VERSION) | sed -re 's/[0-9]*\.([0-9]*).*/\1/')
SUBLEVEL := $(shell echo $(VERSION) | sed -r -e 's/([0-9]*\.){2}([0-9]*).*/\2/' -e 's/[0-9]*\.[0-9]*/0/')
VERSION := $(MAJOR).$(MINOR).$(SUBLEVEL)

#
# tools
#

ifeq ($(MODE),aarch64)
CCPREFIX:=$(LINARO)/host/bin/aarch64-linux-gnu-
CFLAGS+=--sysroot=$(ROOTFSTMP)
LDFLAGS+=--sysroot=$(ROOTFSTMP)
else
CCPREFIX:=
endif

CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar
RANLIB := ${CCPREFIX}ranlib

#
# source file dependencies management
#

DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

%.o: %.c
%.o: %.c %.d
	$(CC) $(DEPFLAGS) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
	mv -f $*.Td $*.d

%.d: ;
.PRECIOUS: %.d

#
# pkg-config configuration
#

#PKG-CONFIG_SILENCE:=--silence-errors

ifeq ($(MODE),x86)
PKG-CONFIG := \
  PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1 \
  PKG_CONFIG_ALLOW_SYSTEM_LIBS=1 \
  PKG_CONFIG_DIR= \
  PKG_CONFIG_LIBDIR=$(ROOTFSTMP)/usr/lib/pkgconfig:$(ROOTFSTMP)/usr/local/lib/pkgconfig:$(ROOTFSTMP)/usr/share/pkgconfig \
  PKG_CONFIG_SYSROOT_DIR=$(ROOTFSTMP) \
  pkg-config \
    $(PKG-CONFIG_SILENCE)
else
PKG-CONFIG := \
  PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1 \
  PKG_CONFIG_ALLOW_SYSTEM_LIBS=1 \
  PKG_CONFIG_DIR= \
  PKG_CONFIG_LIBDIR=$(ROOTFSTMP)/usr/lib/pkgconfig:$(ROOTFSTMP)/usr/share/pkgconfig \
  PKG_CONFIG_SYSROOT_DIR=$(ROOTFSTMP) \
  pkg-config \
    $(PKG-CONFIG_SILENCE)
endif

ifeq ($(shell $(PKG-CONFIG) axiom_user_api >/dev/null || echo fail),fail)
    # GNU make bug 10593: an environment variale can not be
    # inject into sub-shell (only in sub-make)
    # so a default for PKG_CONFIG_PATH can not be set into a makefile
    # (and used by sub-shell)
    $(warning axiom_user_api.pc is not found (PKG_CONFIG_SYSROOT_DIR is '$(ROOTFSTMP)'))
endif

# PKG-CFLAGS equivalent to "pkg-config --cflags ${1}"
PKG-CFLAGS = $(shell $(PKG-CONFIG) --cflags ${1})
# PKG-LDFLAGS equivalent to "pkg-config --libs-only-other --libs-only-L ${1}"
PKG-LDFLAGS = $(shell $(PKG-CONFIG) --libs-only-other --libs-only-L ${1})
# PKG-LDLIBS equvalent to "pkg-config --libs-only-l ${1}"
PKG-LDLIBS = $(shell $(PKG-CONFIG) --libs-only-l ${1})

#
# internal directory structure
#

AXIOM_APPS_INCLUDE_DIR := $(COMMKFILE_DIR)/include
AXIOM_COMMON_CFLAGS := -I$(COMMKFILE_DIR)/axiom_common_library -I$(AXIOM_APPS_INCLUDE_DIR)
AXIOM_COMMON_LDFLAGS := -L$(COMMKFILE_DIR)/axiom_common_library
AXIOM_COMMON_LDLIBS := -laxiom_common
