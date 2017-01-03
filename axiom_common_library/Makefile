
include ../common.mk

LIBS := libaxiom_common.a
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.d)

CLEANFILES = $(LIBS) $(OBJS) $(DEPS)

CFLAGS += -fPIC -Wall $(DFLAGS) -I$(AXIOM_APPS_INCLUDE)

.PHONY: all libs clean install

all: libs

-include $(DEPS)

libs: $(LIBS)

libaxiom_common.a: $(OBJS)
	$(AR) rcs $@ $?
	$(RANLIB) $@

install: libs

clean distclean:
	rm -rf $(CLEANFILES)
