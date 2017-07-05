
include ../common.mk

SUBDIRS := axiom-discovery axiom-pong axiom-spawn axiom-traceroute-reply
SUBDIRS += axiom-netperf-reply axiom-allocator-l1 axiom-session

APPS := axiom-init
SRCS := $(wildcard *.c) $(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.c))
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

LIBS := lib/libaxiom_init_api.so
LIBS_SRCS := $(wildcard lib/*.c)
LIBS_OBJS := $(LIBS_SRCS:.c=.o)
LIBS_DEPS := $(LIBS_SRCS:.c=.d)

CLEANFILES := $(APPS) $(OBJS) $(DEPS) $(foreach LIB,$(LIBS),$(LIB).*) $(LIBS_OBJS) $(LIBS_DEPS)

CFLAGS_LIBS := $(CFLAGS) -fPIC -Wall $(DFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api) \
	$(AXIOM_COMMON_CFLAGS)

CFLAGS += -Wall $(DFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_CFLAGS)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_LDFLAGS)
LDLIBS +=  $(call PKG-LDLIBS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_LDLIBS)

.PHONY: all libs install clean

all: libs $(APPS)

$(LIBS_OBJS): %.o: %.c %.d
	$(CC) $(CFLAGS_LIBS) $(DEPFLAGS) -c -o $@ $<
	mv -f $*.Td $*.d

-include $(DEPS) $(LIBS_DEPS)

axiom-init: $(OBJS)

lib/libaxiom_init_api.so.$(VERSION): $(LIBS_OBJS)
	$(CC) -shared -Wl,--soname,libaxiom_init_api.so.$(MAJOR) \
		$(call PKG-LDFLAGS, axiom_user_api) $(AXIOM_COMMON_LDFLAGS) \
		-o $@ $^ \
		$(call PKG-LDLIBS, axiom_user_api) $(AXIOM_COMMON_LDLIBS)

libs: $(foreach lib,$(LIBS),$(lib).$(VERSION))
ifneq ($(FS),seco)
	for DESTDIR in $(SYSROOT_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/lib/pkgconfig ;\
		mkdir -p $${DESTDIR}/usr/include/axiom ;\
		for LIB in $(LIBS); do \
			LIBNAME=$$(basename $$LIB) ;\
			cp $${LIB}.$(VERSION) $${DESTDIR}/usr/lib/ ;\
			ln -sf $${LIBNAME}.$(VERSION) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR).$(MINOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR).$(MINOR) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR) $${DESTDIR}/usr/lib/$${LIBNAME} ;\
			NAME=$$(basename $$LIB .so | sed -e 's/^lib//') ;\
			D=$$(dirname $$LIB) ;\
			sed -e 's,@PREFIX@,/usr,' $${D}/$${NAME}.pc.in > $${DESTDIR}/usr/lib/pkgconfig/$${NAME}.pc ;\
			cp $(AXIOM_APPS_INCLUDE)/$${NAME}.h $${DESTDIR}/usr/include/axiom ;\
		done ;\
	done
endif
	$(FAKEROOT) $(MAKE) libs-real-install

.PHONY: libs-real-install
libs-real-install:
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/lib/pkgconfig ;\
		mkdir -p $${DESTDIR}/usr/include/axiom ;\
		for LIB in $(LIBS); do \
			cp $${LIB}.$(VERSION) $${DESTDIR}/usr/lib/ ;\
			LIBNAME=$$(basename $$LIB) ;\
			ln -sf $${LIBNAME}.$(VERSION) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR).$(MINOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR).$(MINOR) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR) $${DESTDIR}/usr/lib/$${LIBNAME} ;\
			NAME=$$(basename $$LIB .so | sed -e 's/^lib//') ;\
			D=$$(dirname $$LIB) ;\
			sed -e 's,@PREFIX@,/usr,' $${D}/$${NAME}.pc.in > $${DESTDIR}/usr/lib/pkgconfig/$${NAME}.pc ;\
			cp $(AXIOM_APPS_INCLUDE)/$${NAME}.h $${DESTDIR}/usr/include/axiom ;\
		done ;\
	done

install: all libs
	for DESTDIR in $(TARGET_INST_DIR); do \
		$(FAKEROOT) mkdir -p $${DESTDIR}/usr/bin ;\
		$(FAKEROOT) cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)
