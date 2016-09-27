PWD := $(shell pwd)

AXIOM_APP_INCLUDE := ${PWD}/../include
AXIOM_NIC_INCLUDE := ${PWD}/../../axiom-evi-nic/include

AXIOM_NIC_LIB := ${PWD}/../../axiom-evi-nic/axiom_user_library
AXIOM_RUN_LIB := ${PWD}/../axiom-run/lib
AXIOM_INIT_LIB := ${PWD}/../axiom-init/lib
AXIOM_APP_LIB := ${PWD}

HEADERS := $(AXIOM_NIC_INCLUDE)/*.h $(AXIOM_APP_INCLUDE)/*.h
LIBS := libaxiom_global_allocator.a
TESTS := tests/axiom-test-galloc
CLEANFILES = $(LIBS) $(TESTS) *.o */*.o

#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifdef CCARCH
    BUILDROOT := ${PWD}/../../axiom-evi-buildroot
    DESTDIR := ${BUILDROOT}/output/target
    CCPREFIX := ${BUILDROOT}/output/host/usr/bin/$(CCARCH)-linux-
endif

CC := ${CCPREFIX}gcc
AR := ${CCPREFIX}ar
RANLIB := ${CCPREFIX}ranlib

DFLAGS := -g -DPDEBUG
CFLAGS += -Wall $(DFLAGS) -I$(AXIOM_NIC_INCLUDE) -I$(AXIOM_APP_INCLUDE)
LDFLAGS += -L$(AXIOM_NIC_LIB) -L$(AXIOM_RUN_LIB) -L$(AXIOM_INIT_LIB)
LDFLAGS += -L$(AXIOM_APP_LIB)
LDLIBS += -laxiom_user_api -laxiom_global_allocator -laxiom_run_api

.PHONY: all tests clean install

all: $(LIBS)

tests: $(TESTS)

libaxiom_global_allocator.a: axiom-global-allocator/axiom_global_allocator.o
	$(AR) rcs $@ $?
	$(RANLIB) $@

axiom-global-allocator/axiom_global_allocator.o: axiom-global-allocator/axiom_global_allocator.c $(HEADERS)


tests/axiom-test-galloc: tests/axiom-test-galloc.o

install: all
	cp $(TEST) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)
