AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_DISC := ./axiom-discovery
AXIOM_PONG := ./axiom-pong
AXIOM_BARRIER := ./axiom-barrier
AXIOM_SPAWN := ./axiom-spawn
AXIOM_TR_REPLY := ./axiom-traceroute-reply
AXIOM_NETPERF_REPLY := ./axiom-netperf-reply
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library

APPS := axiom-init
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = $(APPS) *.o $(AXIOM_PONG)/*.o $(AXIOM_TR_REPLY)/*.o $(AXIOM_NETPERF_REPLY)/*.o

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE)
LDFLAGS=

.PHONY: all install clean

all: $(APPS)

axiom-init: axiom-init.o \
	$(AXIOM_DISC)/axiom_discovery_node.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_routing.o \
	$(AXIOM_PONG)/axiom_pong.o $(AXIOM_TR_REPLY)/axiom_traceroute_reply.o $(AXIOM_NETPERF_REPLY)/axiom_netperf_reply.o \
	$(AXIOM_BARRIER)/axiom_barrier.o $(AXIOM_NIC_UL)/libaxiom_user_api.a \
	$(AXIOM_SPAWN)/axiom_spawn.o $(AXIOM_SPAWN)/strlcat.o $(AXIOM_SPAWN)/strlcpy.o

axiom-init.o: axiom-init.c axiom-init.h $(HEADERS)

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_DISC)/%.o:
	cd $(AXIOM_DISC) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	cd $(AXIOM_DISC) && make clean
	rm -rf $(CLEANFILES)



