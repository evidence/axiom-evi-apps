AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ../include

HEADERS := $(AXIOM_INCLUDE)/*.h $(AXIOM_APPS_INCLUDE)/*.h
LIBS := libaxiom_common.a
CLEANFILES = $(LIBS) *.o

PWD := $(shell pwd)
#CCARCH := arm
CCARCH := aarch64
ifdef CCARCH
    BUILDROOT := ${PWD}/../../axiom-evi-buildroot
    DESTDIR := ${BUILDROOT}/output/target
    CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
endif

CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar
RANLIB := ${CCPREFIX}ranlib

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(AXIOM_INCLUDE) -I$(AXIOM_APPS_INCLUDE)

.PHONY: all libs clean install

all: $(LIBS)

libs: $(LIBS)

libaxiom_common.a: strlcat.o strlcpy.o strlist.o logmsg.o daemonize.o
	$(AR) rcs $@ $?
	$(RANLIB) $@

install: all

clean:
	rm -rf $(CLEANFILES)
