
APPS:=axiom-init

SUBDIRS:=axiom-discovery axiom-pong axiom-spawn axiom-traceroute-reply
SUBDIRS+=axiom-allocator-l1 axiom-session

SRCS:=$(wildcard *.c) $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))

include ../simple.mk

CFLAGS += \
  $(call PKG-CFAGS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

LDFLAGS += \
  $(call PKG-LDFLAGS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

LDLIBS += \
  $(call PKG-LDLIBS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

axiom-init: $(OBJS)

all: libs-all
clean: libs-clean
distclean: libs-distclean
mrproper: libs-mrproper
install: libs-install

.PHONY: libs-all libs-clean libs-install libs-distclean libs-mrproper libs

libs-all libs-clean libs-install libs-distclean libs-mrproper libs:
	$(MAKE) -C lib $(subst libs-,,$@)
