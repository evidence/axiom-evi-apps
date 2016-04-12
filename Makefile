APPS_DIR := axiom-init axiom-recv-small axiom-send-small axiom-whoami axiom-info
APPS_DIR += axiom-ping axiom-ping-reply
CLEAN_DIR := $(addprefix _clean_, $(APPS_DIR))
INSTALL_DIR := $(addprefix _install_, $(APPS_DIR))

PWD := $(shell pwd)
BUILDROOT := ${PWD}/../axiom-evi-buildroot
DESTDIR := ${BUILDROOT}/output/target
CCPREFIX := ${BUILDROOT}/output/host/usr/bin/arm-linux-

DFLAGS := -g -DPDEBUG

.PHONY: all clean install $(APPS_DIR) $(CLEAN_DIR) $(INSTALL_DIR)

all: $(APPS_DIR)

$(APPS_DIR):
	cd $@ && make CCPREFIX=$(CCPREFIX) DFLAGS="$(DFLAGS)"


install: $(INSTALL_DIR)

$(INSTALL_DIR):
	cd $(subst _install_,,$@) && make install CCPREFIX=$(CCPREFIX) DESTDIR=$(DESTDIR) DFLAGS="$(DFLAGS)"


clean: $(CLEAN_DIR)

$(CLEAN_DIR): _clean_%:
	cd $(subst _clean_,,$@) && make clean

