AXIOM_INCLUDE := ../../axiom-evi-nic/include
AXIOM_NIC_UL := ../../axiom-evi-nic/axiom_user_library
AXIOM_RUN_LIB := ../axiom-run/lib

AXIOM_OVERLAY := ../../scripts/buildroot/overlay
AXIOM_ALLOC_LDS := $(AXIOM_OVERLAY)/usr/axiom-evi-allocator-lib/xs_map64.lds

APPS := axiom-rdma
HEADERS := $(AXIOM_INCLUDE)/*.h
CLEANFILES = $(APPS) *.o

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
CC := ${CCPREFIX}gcc

DFLAGS := -g -DPDEBUG
CFLAGS += -g -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE)
LDFLAGS += -L$(PWD)/$(AXIOM_OVERLAY)/usr/lib -Wl,-T$(AXIOM_ALLOC_LDS)
LDFLAGS += -L$(PWD)/$(AXIOM_NIC_UL) -L$(PWD)/$(AXIOM_RUN_LIB)
LDLIBS += -laxiom_allocator -levi_lmm -laxiom_user_api -laxiom_run_api

.PHONY: all clean

all: $(APPS)

axiom-rdma: axiom-rdma.o

axiom-rdma.o: axiom-rdma.c $(HEADERS)

$(AXIOM_NIC_UL)/%:
	cd $(AXIOM_NIC_UL) && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)

