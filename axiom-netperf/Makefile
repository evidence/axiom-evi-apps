
include ../common.mk

APPS := axiom-netperf

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

axion-netperf: $(OBJS)

ifeq ($(FS),x86)
MYPREFIX:=/usr/local
else
MYPREFIX:=/usr
endif

install: all
	for DESTDIR in $(TARGET_INST_DIR); do \
		$(FAKEROOT) mkdir -p $${DESTDIR}$(MYPREFIX)/bin ;\
		$(FAKEROOT) cp $(APPS) $${DESTDIR}$(MYPREFIX)/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)
