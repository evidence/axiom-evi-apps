
include ../common.mk

ifndef SRCS
SRCS := $(wildcard *.c)
endif
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
CLEANFILES = $(APPS) $(OBJS) $(DEPS)

CFLAGS += -Wall \
	$(DFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api) \
	$(AXIOM_COMMON_CFLAGS)

LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api) \
	$(AXIOM_COMMON_LDFLAGS)

LDLIBS += $(call PKG-LDLIBS, axiom_user_api) \
	$(AXIOM_COMMON_LDLIBS)

.PHONY: all clean install distclen mrproper

all: $(APPS)

-include $(DEPS)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin ;\
	cp $(APPS) $(DESTDIR)$(PREFIX)/bin

clean distclean mrproper:
	rm -rf $(CLEANFILES)
