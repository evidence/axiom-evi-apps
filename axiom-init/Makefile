AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ../include
AXIOM_DISC := ./axiom-discovery
AXIOM_PONG := ./axiom-pong
AXIOM_SPAWN := ./axiom-spawn
AXIOM_SESSION := ./axiom-session
AXIOM_TR_REPLY := ./axiom-traceroute-reply
AXIOM_NETPERF_REPLY := ./axiom-netperf-reply
AXIOM_ALLOCATOR := ./axiom-allocator
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library
AXIOM_COMMON_UL := ../axiom_common_library

APPS := axiom-init
LIBS := lib/libaxiom_init_api.a

HEADERS := $(AXIOM_INCLUDE)/*.h $(AXIOM_APPS_INCLUDE)/*.h
CLEANFILES = $(APPS) $(LIBS) *.o lib/*.o $(AXIOM_PONG)/*.o $(AXIOM_TR_REPLY)/*.o
CLEANFILES += $(AXIOM_NETPERF_REPLY)/*.o $(AXIOM_SESSION)/*.o $(AXIOM_SPAWN)/*.o
CLEANFILES += $(AXIOM_ALLOCATOR)/*.o

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE) -I$(PWD)/$(AXIOM_APPS_INCLUDE)
LDFLAGS=

.PHONY: all install clean

all: $(APPS) $(LIBS)

axiom-init: axiom-init.o \
	$(AXIOM_DISC)/axiom_discovery_node.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_routing.o \
	$(AXIOM_PONG)/axiom_pong.o $(AXIOM_TR_REPLY)/axiom_traceroute_reply.o $(AXIOM_NETPERF_REPLY)/axiom_netperf_reply.o \
	$(AXIOM_ALLOCATOR)/axiom_allocator.o \
	$(AXIOM_NIC_UL)/libaxiom_user_api.a $(AXIOM_SPAWN)/axiom_spawn.o $(AXIOM_COMMON_UL)/libaxiom_common.a \
	$(AXIOM_SESSION)/axiom_session.o $(AXIOM_COMMON_UL)/libaxiom_common.a

axiom-init.o: axiom-init.c axiom-init.h $(HEADERS)

lib/libaxiom_init_api.a: lib/execvpe.o lib/allocator.o
	$(AR) rcs $@ $?

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_COMMON_UL)/%:
	cd $(AXIOM_COMMON_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_DISC)/%.o:
	cd $(AXIOM_DISC) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	cd $(AXIOM_DISC) && make clean
	rm -rf $(CLEANFILES)



