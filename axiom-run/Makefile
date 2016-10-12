AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ..//include
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library
AXIOM_APPS_UL := ../axiom-init/lib
AXIOM_COMMON_UL := ../axiom_common_library

APPS := axiom-run
TAPPS := test/myinfo test/mybar test/myexch test/myrpc
LIBS := lib/libaxiom_run_api.a

CLEANFILES = $(APPS) $(TAPPS) $(LIBS) *.o test/*.o lib/*.o

PWD := $(shell pwd)

#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall -D_GNU_SOURCE $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE) -I$(PWD)/$(AXIOM_APPS_INCLUDE)
LDFLAGS += -L $(AXIOM_COMMON_UL) -L $(AXIOM_NIC_UL) -L $(AXIOM_APPS_UL)
LDLIBS += -laxiom_user_api -laxiom_init_api -laxiom_common -lpthread

.PHONY: all clean

all: $(APPS) $(LIBS) $(TAPPS)

axiom-run: axiom-run.o master.o slave.o rpcfuncs.o axiom_allocator_l1_l2.o $(AXIOM_COMMON_UL)/libaxiom_common.a $(AXIOM_APPS_UL)/libaxiom_init_api.a

lib/libaxiom_run_api.a: lib/sync.o lib/rpc.o
	$(AR) rcs $@ $?

test/myinfo: test/myinfo.o

test/myexch: test/myexch.o
	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -o test/myexch test/myexch.o -laxiom_user_api -lpthread

test/mybar: test/mybar.o lib/libaxiom_run_api.a
	$(CC) -L$(PWD)/lib -o test/mybar test/mybar.o -laxiom_run_api

test/myrpc: test/myrpc.o lib/libaxiom_run_api.a
	$(CC) -L$(PWD)/lib -o test/myrpc test/myrpc.o -laxiom_run_api

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_APPS_UL)/%:
	cd $(AXIOM_APPS_UL)/.. && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

$(AXIOM_COMMON_UL)/%:
	cd $(AXIOM_COMMON_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)

