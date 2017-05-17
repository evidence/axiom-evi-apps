
include ../common.mk

APPS := axiom-ethtap

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
CLEANFILES = $(APPS) $(OBJS) $(DEPS)

CFLAGS += -Wall $(DFLAGS) $(call PKG-CFLAGS, axiom_user_api) $(AXIOM_COMMON_CFLAGS)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api) -pthread $(AXIOM_COMMON_LDFLAGS)
LDLIBS += $(call PKG-LDLIBS, axiom_user_api) $(AXIOM_COMMON_LDLIBS)

.PHONY: all clean install

all: $(APPS)

-include $(DEPS)

axiom-ethtap: $(OBJS)

install: all
	for DESTDIR in $(TARGET_INST_DIR); do \
		$(FAKEROOT) mkdir -p $${DESTDIR}/usr/bin ;\
		$(FAKEROOT) cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)
