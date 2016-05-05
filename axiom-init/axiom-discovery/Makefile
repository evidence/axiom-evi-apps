AXIOM_INCLUDE := ../../../axiom-evi-nic/include

OBJS := axiom_discovery_node.o axiom_discovery_protocol.o axiom_routing.o
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

axiom_discovery_node.o: axiom_discovery_node.c ../axiom-init.h axiom_routing.h axiom_discovery_protocol.h $(HEADERS)

axiom_discovery_protocol.o: axiom_discovery_protocol.c axiom_discovery_protocol.h $(HEADERS)

axiom_routing.o: axiom_routing.c axiom_discovery_protocol.h axiom_routing.h $(HEADERS)

clean:
	rm -rf $(CLEANFILES)

