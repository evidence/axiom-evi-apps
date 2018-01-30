
.PHONY: clean build install distclean mrproper

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

clean distclean mrproper:
	rm -f $(OBJS) $(DEPS) $(EXECS)

install: build
	mkdir -p $(DESTDIR)/opt/axiom/tests_axiom
	cp $(EXECS) $(DESTDIR)/opt/axiom/tests_axiom
