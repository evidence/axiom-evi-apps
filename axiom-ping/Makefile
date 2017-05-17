
include ../common.mk

APPS := axiom-ping

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
CLEANFILES = $(APPS) $(OBJS) $(DEPS)

CFLAGS += -Wall $(DFLAGS) $(call PKG-CFLAGS, axiom_user_api)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api) 
LDLIBS += -pthread $(call PKG-LDLIBS, axiom_user_api)

.PHONY: all clean install

all: $(APPS)

-include $(DEPS)

axiom-ping: $(OBJS)

install: all
	for DESTDIR in $(TARGET_INST_DIR); do \
		$(FAKEROOT) mkdir -p $${DESTDIR}/usr/bin ;\
		$(FAKEROOT) cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)

