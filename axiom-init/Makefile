AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ../include
AXIOM_ALLOC_INCLUDE := ../../axiom-allocator/include
AXIOM_ALLOC_LIB := ../../axiom-allocator/src/.libs
AXIOM_DISC := ./axiom-discovery
AXIOM_PONG := ./axiom-pong
AXIOM_SPAWN := ./axiom-spawn
AXIOM_SESSION := ./axiom-session
AXIOM_TR_REPLY := ./axiom-traceroute-reply
AXIOM_NETPERF_REPLY := ./axiom-netperf-reply
AXIOM_ALLOCATOR := ./axiom-allocator-l1
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library
AXIOM_COMMON_UL := ../axiom_common_library

APPS := axiom-init
LIBS := lib/libaxiom_init_api.a

HEADERS := $(AXIOM_INCLUDE)/*.h $(AXIOM_APPS_INCLUDE)/*.h $(AXIOM_ALLOC_INCLUDE)/*.h
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
CFLAGS += -I$(PWD)/$(AXIOM_ALLOC_INCLUDE)
LDFLAGS += -L$(PWD)/$(AXIOM_NIC_UL) -L$(PWD)/$(AXIOM_ALLOC_LIB) -L$(AXIOM_COMMON_UL)
LDLIBS += -laxiom_user_api -laxiom_allocator_l1 -laxiom_common

.PHONY: all libs install clean

all: $(APPS) $(LIBS)

libs: $(LIBS)

axiom-init: axiom-init.o \
	$(AXIOM_DISC)/axiom_discovery_node.o $(AXIOM_DISC)/axiom_discovery_protocol.o $(AXIOM_DISC)/axiom_routing.o \
	$(AXIOM_PONG)/axiom_pong.o $(AXIOM_TR_REPLY)/axiom_traceroute_reply.o $(AXIOM_NETPERF_REPLY)/axiom_netperf_reply.o \
	$(AXIOM_ALLOCATOR)/axiom_allocator_l1.o \
	$(AXIOM_SPAWN)/axiom_spawn.o $(AXIOM_SESSION)/axiom_session.o

axiom-init.o: axiom-init.c axiom-init.h $(HEADERS)

lib/libaxiom_init_api.a: lib/execvpe.o
	$(AR) rcs $@ $?

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)" CFLAGS="$(CFLAGS)"

$(AXIOM_COMMON_UL)/%:
	cd $(AXIOM_COMMON_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)" CFLAGS="$(CFLAGS)"

$(AXIOM_DISC)/%.o:
	cd $(AXIOM_DISC) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)" CFLAGS="$(CFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	cd $(AXIOM_DISC) && make clean
	rm -rf $(CLEANFILES)



