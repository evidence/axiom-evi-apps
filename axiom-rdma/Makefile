
include ../common.mk

APPS := axiom-rdma

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
CLEANFILES = $(APPS) $(OBJS) $(DEPS)

CFLAGS += -Wall $(DFLAGS) $(call PKG-CFLAGS, axiom_user_api axiom_allocator axiom_run_api)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api axiom_allocator evi_lmm axiom_run_api) 
LDLIBS += $(call PKG-LDLIBS, axiom_user_api axiom_allocator evi_lmm axiom_run_api)

.PHONY: all clean install

all: $(APPS)

-include $(DEPS)

axiom-rdma: $(OBJS)

install: all
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/bin ;\
		cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)

