
.PHONY: clean build install distclean install-real

include ../../common.mk

SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
DEPS=$(OBJS:.o=.d)
EXECS=$(OBJS:.o=)

CFLAGS += -g -O3 -finline-functions -fomit-frame-pointer -Wall -std=gnu11
LDFLAGS += -pthread

CFLAGS += $(call PKG-CFLAGS, axiom_user_api axiom_run_api axiom_allocator evi_lmm)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api axiom_run_api axiom_allocator evi_lmm)
LDLIBS += $(call PKG-LDLIBS, axiom_user_api axiom_run_api axiom_allocator evi_lmm) ../common/libtest.a

build: $(OBJS) $(EXECS)

clean distclean:
	rm -f $(OBJS) $(DEPS) $(EXECS)

install: build
	$(FAKEROOT) $(MAKE) install-real

install-real:
	for DESTDIR in $(TARGET_DIR); do \
	    mkdir -p $${DESTDIR}/root/tests_axiom ;\
	    cp $(EXECS) $${DESTDIR}/root/tests_axiom ;\
	done
