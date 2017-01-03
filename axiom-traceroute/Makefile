
include ../common.mk

APPS := axiom-traceroute

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
CLEANFILES = $(APPS) $(OBJS) $(DEPS)

CFLAGS += -Wall $(DFLAGS) $(call PKG-CFLAGS, axiom_user_api)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api) 
LDLIBS += $(call PKG-LDLIBS, axiom_user_api)

.PHONY: all clean install

all: $(APPS)

-include $(DEPS)

axiom-traceroute: $(OBJS)

install: all
	for DESTDIR in $(TARGET_DIR); do \
		mkdir -p $${DESTDIR}/usr/bin ;\
		cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean:
	rm -rf $(CLEANFILES)
