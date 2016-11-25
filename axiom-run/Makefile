AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ..//include
AXIOM_ALLOC_INCLUDE := ../../axiom-allocator/include
AXIOM_ALLOC_LIB := ../../axiom-allocator/src/.libs
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library
AXIOM_APPS_UL := ../axiom-init/lib
AXIOM_COMMON_UL := ../axiom_common_library
AXIOM_OVERLAY := ../../scripts/buildroot/overlay
AXIOM_ALLOC_LDS := $(AXIOM_OVERLAY)/usr/axiom-evi-allocator-lib/xs_map64.lds

APPS := axiom-run 
TAPPS := test/myinfo test/mybar test/myexch test/myrpc test/myputbulk test/myrdma
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
CFLAGS += -I$(PWD)/$(AXIOM_ALLOC_INCLUDE)
LDFLAGS += -L $(AXIOM_COMMON_UL) -L $(AXIOM_NIC_UL) -L $(AXIOM_APPS_UL)
LDFLAGS += -L$(PWD)/$(AXIOM_ALLOC_LIB)
LDLIBS += -laxiom_user_api -laxiom_init_api -laxiom_common -lpthread
LDLIBS += -laxiom_allocator_l2

.PHONY: all libs clean

all: $(APPS) $(LIBS) $(TAPPS)

libs: $(LIBS)

axiom-run: axiom-run.o master.o slave.o rpcfuncs.o

lib/%.o: lib/%.c
	$(CC) -c -fPIC $(CFLAGS) -o $@ $?

lib/libaxiom_run_api.a: lib/sync.o lib/rpc.o lib/nodes.o
	$(AR) rcs $@ $?

test/myinfo: test/myinfo.o
	$(CC) -L$(PWD)/lib -o test/myinfo test/myinfo.o -laxiom_run_api

test/myexch: test/myexch.o
	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -o test/myexch test/myexch.o -laxiom_user_api -lpthread

test/myrdma: test/myrdma.o
	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -L$(PWD)/$(AXIOM_OVERLAY)/usr/lib -Wl,-T$(AXIOM_ALLOC_LDS) -o test/myrdma test/myrdma.o -laxiom_allocator -levi_lmm -laxiom_user_api -lpthread

test/mybar: test/mybar.o lib/libaxiom_run_api.a
	$(CC) -L$(PWD)/lib -o test/mybar test/mybar.o -laxiom_run_api

test/myrpc: test/myrpc.o lib/libaxiom_run_api.a
	$(CC) -L$(PWD)/lib -o test/myrpc test/myrpc.o -laxiom_run_api

test/myputbulk: test/myputbulk.o
	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -o test/myputbulk test/myputbulk.o -laxiom_user_api

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

