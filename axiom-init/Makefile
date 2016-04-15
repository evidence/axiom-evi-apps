AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_DISC := ./axiom-discovery
AXIOM_PONG := ./axiom-pong
AXIOM_TR_REPLY := ./axiom-traceroute-reply
AXIOM_NETPERF_REPLY := ./axiom-netperf-reply
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library

APPS := axiom-init
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = $(APPS) *.o $(AXIOM_PONG)/*.o $(AXIOM_TR_REPLY)/*.o $(AXIOM_NETPERF_REPLY)/*.o

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-
CC := ${CCPREFIX}gcc

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE)

.PHONY: all install clean

all: $(APPS)

axiom-init: axiom-init.o $(AXIOM_DISC)/axiom_discovery_node.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_route_compute.o $(AXIOM_DISC)/axiom_route_delivery.o $(AXIOM_DISC)/axiom_route_set.o $(AXIOM_PONG)/axiom_pong.o $(AXIOM_TR_REPLY)/axiom_traceroute_reply.o $(AXIOM_NETPERF_REPLY)/axiom_netperf_reply.o $(AXIOM_NIC_UL)/axiom_user_api.a

axiom-init.o: axiom-init.c $(AXIOM_DISC)/axiom_discovery_node.h $(AXIOM_PONG)/axiom_pong.h $(AXIOM_TR_REPLY)/axiom_traceroute_reply.h $(AXIOM_NETPERF_REPLY)/axiom_netperf_reply.h $(HEADERS)

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_DISC)/%.o:
	cd $(AXIOM_DISC) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	cd $(AXIOM_DISC) && make clean
	rm -rf $(CLEANFILES)



