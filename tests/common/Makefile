
.PHONY: clean build install

include ../../common.mk

CFLAGS += -g -O3 -finline-functions -fomit-frame-pointer -Wall -Wno-unused-function -std=gnu11

SOURCES=$(wildcard *.c)
OBJS=$(SOURCES:.c=.o)
DEPS=$(OBJS:.o=.d)

build: $(OBJS) libtest.a

libtest.a: $(OBJS)
	$(AR) rcs libtest.a $(OBJS)

install: build
	for DESTDIR in $(TARGET_DIR); do \
	    mkdir -p $${DESTDIR}/root/tests_axiom; \
	    cp ../scripts/* $${DESTDIR}/root/tests_axiom; \
	done

clean:
	rm -f $(OBJS) $(DEPS) libtest.a
