AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library

APPS := axiom-traceroute
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = $(APPS) *.o

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-
CC := ${CCPREFIX}gcc

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE)

.PHONY: all clean

all: $(APPS)

axiom-netperf: axiom-netperf.o $(AXIOM_NIC_UL)/axiom_user_api.a

axiom-netperf.o: axiom-netperf.c $(HEADERS)

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)

