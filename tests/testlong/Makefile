
.PHONY: clean build install

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

clean:
	rm -f $(OBJS) $(DEPS) $(EXECS)

install: build
	for DESTDIR in $(TARGET_DIR); do \
	    mkdir -p $${DESTDIR}/root/tests_axiom; \
	    cp $(EXECS) $${DESTDIR}/root/tests_axiom; \
	done
