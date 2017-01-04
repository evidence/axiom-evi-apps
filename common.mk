
#
# default architecture
#

CCARCH := aarch64

#
# version numbers
#

VERSION := $(shell git describe --tags --dirty | sed -re 's/^axiom-v([0-9.]*).*/\1/')
MAJOR := $(shell echo $(VERSION) | sed -e 's/\..*//')
MINOR := $(shell echo $(VERSION) | sed -re 's/[0-9]*\.([0-9]*).*/\1/')
SUBLEVEL := $(shell echo $(VERSION) | sed -r -e 's/([0-9]*\.){2}([0-9]*).*/\2/' -e 's/[0-9]*\.[0-9]*/0/')
VERSION := $(MAJOR).$(MINOR).$(SUBLEVEL)

#
# variuos output directories
#

COMMKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#BUILDROOT := ${COMMKFILE_DIR}/../axiom-evi-buildroot
OUTPUT_DIR := ${COMMKFILE_DIR}/../output
TARGET_DIR := $(realpath ${OUTPUT_DIR}/target)
SYSROOT_DIR := $(realpath ${OUTPUT_DIR}/staging)
HOST_DIR := $(realpath ${OUTPUT_DIR}/host)

#
# pkg-config configuration
#

#PKG-CONFIG_SILENCE:=--silence-errors

PKG-CONFIG := PKG_CONFIG_ALLOW_SYSTEM_CFLAGS=1 PKG_CONFIG_ALLOW_SYSTEM_LIBS=1 PKG_CONFIG_DIR= PKG_CONFIG_LIBDIR=$(SYSROOT_DIR)/usr/lib/pkgconfig:$(SYSROOT_DIR)/usr/share/pkgconfig PKG_CONFIG_SYSROOT_DIR=$(SYSROOT_DIR) pkg-config $(PKG-CONFIG_SILENCE)
ifeq ($(shell $(PKG-CONFIG) axiom_user_api >/dev/null || echo fail),fail)
    # GNU make bug 10593: an environment variale can not be inject into sub-shell (only in sub-make)
    # so a default for PKG_CONFIG_PATH can not be set into a makefile (and used by sub-shell)
    $(warning axiom_user_api.pc is not found (PKG_CONFIG_SYSROOT_DIR is '$(SYSROOT_DIR)' $(TARGET_DIR)))
endif

# PKG-CFLAGS equivalent to "pkg-config --cflags ${1}" into the cross sysroot
PKG-CFLAGS = $(shell $(PKG-CONFIG) --cflags ${1})
# PKG-LDFLAGS equivalent to "pkg-config --libs-only-other --libs-only-L ${1}" into the cross sysroot
PKG-LDFLAGS = $(shell $(PKG-CONFIG) --libs-only-other --libs-only-L ${1})
# PKG-LDLIBS equvalent to "pkg-config --libs-only-l ${1}" into the cross sysroot
PKG-LDLIBS = $(shell $(PKG-CONFIG) --libs-only-l ${1})

#
# internal directory structure
#

AXIOM_APPS_INCLUDE := $(COMMKFILE_DIR)/include

AXIOM_COMMON_CFLAGS := -I$(COMMKFILE_DIR)/axiom_common_library -I$(AXIOM_APPS_INCLUDE)
AXIOM_COMMON_LDFLAGS := -L$(COMMKFILE_DIR)/axiom_common_library
AXIOM_COMMON_LDLIBS := -laxiom_common

#
# tools
#

CCPREFIX := ${HOST_DIR}/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar
RANLIB := ${CCPREFIX}ranlib

#DFLAGS := -g -DPDEBUG
DFLAGS := -g

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
