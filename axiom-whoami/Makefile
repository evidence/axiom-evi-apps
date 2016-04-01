AXIOM_INCLUDE := ../../axiom-docs/include
AXIOM_NIC_UL := ../../axiom-nic/axiom_user_library

APPS := axiom-whoami
CLEANFILES = $(APPS) *.o
CFLAGS += -Wall -g -DPDEBUG -I$(AXIOM_INCLUDE)

PWD := $(shell pwd)
#MKFILE_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
BUILDROOT := ${PWD}/../../axiom-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-
CC := ${CCPREFIX}gcc

.PHONY: all clean

all: $(APPS)

axiom-whoami: axiom-whoami.o $(AXIOM_NIC_UL)/axiom_user_api.o

axiom-whoami.o: axiom-whoami.c

install: $(APPS)
	cp $(APPS) $(DESTDIR)/usr/bin/

clean:
	rm -rf $(CLEANFILES)

