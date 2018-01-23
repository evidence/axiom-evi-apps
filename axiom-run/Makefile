
APPS := axiom-run 

include ../simple.mk

CFLAGS += \
  -D_GNU_SOURCE \
  $(call PKG-CFLAGS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

LDFLAGS += \
  $(call PKG-LDFLAGS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

LDLIBS += \
  -lpthread \
  -lrt \
  $(call PKG-LDLIBS, axiom_init_api axiom_run_api axiom_allocator evi_lmm)

axiom-run: $(OBJS)

all: libs-all
clean: libs-clean
distclean: libs-distclean
mrproper: libs-mrproper
install: libs-install

.PHONY: libs-all libs-clean libs-install libs-distclean libs-mrproper libs

libs-all libs-clean libs-install libs-distclean libs-mrproper libs:
	$(MAKE) -C lib $(subst libs-,,$@)
