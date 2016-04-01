AXIOM_INCLUDE := ../../axiom-docs/include
AXIOM_DISC := axiom-discovery
AXIOM_SIM := $(AXIOM_DISC)/axiom_simulator
AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom-init
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I$(AXIOM_INCLUDE) -I$(AXIOM_DISC)

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := ${PWD}/../../axiom-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-
CC := ${CCPREFIX}gcc

.PHONY: all clean

all: $(APPS)

axiom-init: axiom-init.o  $(AXIOM_DISC)/axiom_node_code.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_route_compute.o $(AXIOM_DISC)/axiom_route_delivery.o $(AXIOM_DISC)/axiom_route_set.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom-init.o: axiom-init.c $(AXIOM_INCLUDE)/axiom_nic_types.h $(AXIOM_INCLUDE)/axiom_sim_topology.h $(AXIOM_DISC)/axiom_node_code.h

install: $(APPS)
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)



