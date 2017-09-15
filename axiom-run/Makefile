
include ../common.mk

APPS := axiom-run 
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

LIBS := lib/libaxiom_run_api.so
LIBS_SRCS := $(wildcard lib/*.c)
LIBS_OBJS := $(LIBS_SRCS:.c=.o)
LIBS_DEPS := $(LIBS_SRCS:.c=.d)

CLEANFILES = $(APPS) $(OBJS) $(DEPS) $(foreach LIB,$(LIBS),$(LIB).*) $(LIBS_OBJS) $(LIBS_DEPS)

CFLAGS_LIBS := $(CFLAGS) -fPIC -Wall -D_GNU_SOURCE $(DFLAGS) \
	$(AXIOM_COMMON_CFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api)

CFLAGS += -Wall -D_GNU_SOURCE $(DFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_CFLAGS)
LDFLAGS += $(call PKG-LDFLAGS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_LDFLAGS)
LDLIBS += -lpthread \
	$(call PKG-LDLIBS, axiom_user_api axiom_init_api axiom_run_api axiom_allocator evi_lmm) \
	$(AXIOM_COMMON_LDLIBS)

.PHONY: all libs clean install clean

all: libs $(APPS)

$(LIBS_OBJS): %.o: %.c %.d
	$(CC) $(CFLAGS_LIBS) $(DEPFLAGS) -c -o $@ $<
	mv -f $*.Td $*.d

-include $(DEPS) $(LIBS_DEPS)

axiom-run: $(OBJS)

lib/libaxiom_run_api.so.$(VERSION): $(LIBS_OBJS)
	$(CC) -shared -Wl,--soname,libaxiom_run_api.so.$(MAJOR) \
		$(call PKG-LDFLAGS, axiom_user_api) $(AXIOM_COMMON_LDFLAGS) \
		-o $@ $^ \
		$(call PKG-LDLIBS, axiom_user_api) $(AXIOM_COMMON_LDLIBS)

libs: $(foreach lib,$(LIBS),$(lib).$(VERSION))
ifeq ($(FS),br)
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
ifeq ($(FS),x86)
ifdef FROMDEB
	$(MAKE) TARGET_INST_DIR=$(TARGET_INST_DIR) PREFIX=/usr/local libs-real-install
else
	sudo $(MAKE) TARGET_INST_DIR=/ PREFIX=/usr/local libs-real-install
endif
else
	$(FAKEROOT) $(MAKE) PREFIX=/usr libs-real-install
endif

.PHONY: libs-real-install
libs-real-install:
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}$(PREFIX)/lib/pkgconfig ;\
		mkdir -p $${DESTDIR}$(PREFIX)/include/axiom ;\
		for LIB in $(LIBS); do \
			cp $${LIB}.$(VERSION) $${DESTDIR}$(PREFIX)/lib/ ;\
			LIBNAME=$$(basename $$LIB) ;\
			ln -sf $${LIBNAME}.$(VERSION) $${DESTDIR}$(PREFIX)/lib/$${LIBNAME}.$(MAJOR).$(MINOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR).$(MINOR) $${DESTDIR}$(PREFIX)/lib/$${LIBNAME}.$(MAJOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR) $${DESTDIR}$(PREFIX)/lib/$${LIBNAME} ;\
			NAME=$$(basename $$LIB .so | sed -e 's/^lib//') ;\
			D=$$(dirname $$LIB) ;\
			sed -e 's,@PREFIX@,/usr,' $${D}/$${NAME}.pc.in > $${DESTDIR}$(PREFIX)/lib/pkgconfig/$${NAME}.pc ;\
			cp $(AXIOM_APPS_INCLUDE)/$${NAME}.h $${DESTDIR}$(PREFIX)/include/axiom ;\
		done ;\
	done

install: all libs
ifeq ($(FS),x86)
ifdef TARGET_INST_DIR
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/local/bin ;\
		cp $(APPS) $${DESTDIR}/usr/local/bin/ ;\
	done
else
	for DESTDIR in /; do \
		sudo mkdir -p $${DESTDIR}/usr/local/bin ;\
		sudo cp $(APPS) $${DESTDIR}/usr/local/bin/ ;\
	done
endif
else
	for DESTDIR in $(TARGET_INST_DIR); do \
		$(FAKEROOT) mkdir -p $${DESTDIR}/usr/bin ;\
		$(FAKEROOT) cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done
endif

clean distclean:
	rm -rf $(CLEANFILES)
