AXIOM_INCLUDE := ../axiom-evi-nic/include
AXIOM_APPS_INCLUDE := ./include
AXIOM_ALLOC_INCLUDE := ../axiom-allocator/include
APPS_DIR := axiom-init axiom-recv axiom-send axiom-whoami axiom-info
APPS_DIR += axiom-ping axiom-traceroute axiom-netperf axiom-rdma axiom-run axiom-utility
LIBS_DIR := axiom_common_library
CLEAN_DIR := $(addprefix _clean_, $(APPS_DIR) $(LIBS_DIR))
INSTALL_DIR := $(addprefix _install_, $(APPS_DIR))

PWD := $(shell pwd)
#CCARCH := arm
CCARCH := aarch64
BUILDROOT := ${PWD}/../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-

ifndef AXIOMHOME
    AXIOMHOME=$(realpath ${PWD}/..)
endif
ifndef HOST_DIR
   HOST_DIR=${AXIOMHOME}/output
endif

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(PWD)/$(AXIOM_INCLUDE) -I$(PWD)/$(AXIOM_APPS_INCLUDE)
CFLAGS += -I$(PWD)/$(AXIOM_ALLOC_INCLUDE)

.PHONY: all clean install installhost $(APPS_DIR) $(LIBS_DIR) $(CLEAN_DIR) $(INSTALL_DIR)

all: $(LIBS_DIR) $(APPS_DIR)

libs: $(LIBS_DIR)
	cd axiom-init && CFLAGS="$(CFLAGS)" make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)" libs
	cd axiom-run && CFLAGS="$(CFLAGS)" make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)" libs

$(APPS_DIR):: $(LIBS_DIR)

axiom-run:: axiom-init

$(LIBS_DIR) $(APPS_DIR)::
	cd $@ && CFLAGS="$(CFLAGS)" make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"

install: all $(INSTALL_DIR)

$(INSTALL_DIR):
	cd $(subst _install_,,$@) &&  CFLAGS="$(CFLAGS)" make install CCPREFIX=$(CCPREFIX) DESTDIR=$(DESTDIR) DFLAGS="$(DFLAGS)"


clean: $(CLEAN_DIR)

$(CLEAN_DIR): _clean_%:
	cd $(subst _clean_,,$@) && make clean


installhost:
	mkdir -p ${HOST_DIR}
	cp -r include ${HOST_DIR}
	mkdir -p ${HOST_DIR}/lib
	cp axiom-init/lib/libaxiom_init_api.a ${HOST_DIR}/lib
	cp axiom-run/lib/libaxiom_run_api.a ${HOST_DIR}/lib
	cp axiom_common_library/libaxiom_common.a ${HOST_DIR}/lib
