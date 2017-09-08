
.PHONY: clean build install distclean install-real

include ../../common.mk

SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
DEPS=$(OBJS:.o=.d)
EXECS=$(OBJS:.o=)

CFLAGS += -g -O3 -finline-functions -fomit-frame-pointer -Wall -std=gnu11
LDFLAGS += -pthread

CFLAGS += $(call PKG-CFLAGS, axiom_user_api axiom_run_api)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api axiom_run_api)
LDLIBS += $(call PKG-LDLIBS, axiom_user_api axiom_run_api) ../common/libtest.a

build: $(OBJS) $(EXECS)

clean distclean:
	rm -f $(OBJS) $(DEPS) $(EXECS)

install: build
	$(FAKEROOT) $(MAKE) install-real

ifeq ($(FS),x86)
ifdef FROMDEB
SUDO :=
else
SUDO := sudo
endif
else
SUDO :=
endif

#default if not overwritten
DESTDIR=$(TARGET_DIR)

install-real:
	$(SUDO) mkdir -p $${DESTDIR}/opt/axiom/tests_axiom
	$(SUDO) cp $(EXECS) $${DESTDIR}/opt/axiom/tests_axiom
