AXIOM_INCLUDE := ../../../axiom-evi-nic/include

OBJS := axiom_discovery_node.o axiom_discovery_protocol.o axiom_route_compute.o axiom_route_delivery.o axiom_route_set.o
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = *.o

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(AXIOM_INCLUDE)
LDLIBS += -pthread -lm

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc


.PHONY: all clean

all: $(OBJS)

axiom_discovery_node.o: axiom_discovery_node.c ../axiom-init.h axiom_route_compute.h axiom_route_delivery.h axiom_route_set.h axiom_discovery_protocol.h $(HEADERS)

axiom_discovery_protocol.o: axiom_discovery_protocol.c axiom_discovery_protocol.h $(HEADERS)

axiom_route_compute.o: axiom_route_compute.c axiom_discovery_protocol.h axiom_route_compute.h $(HEADERS)

axiom_route_delivery.o: axiom_route_delivery.c axiom_route_delivery.h axiom_discovery_protocol.h axiom_route_compute.h $(HEADERS)

axiom_route_set.o: axiom_route_set.c axiom_discovery_protocol.h axiom_route_compute.h axiom_route_set.h $(HEADERS)


clean:
	rm -rf $(CLEANFILES)

