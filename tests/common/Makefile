
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

clean:
	rm -f $(OBJS) $(DEPS) libtest.a
