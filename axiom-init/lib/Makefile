include ../../common.mk

LIBS := libaxiom_init_api.so
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)

CLEANFILES := $(OBJS) $(DEPS) $(LIBS_OBJS) $(LIBS_DEPS) \
	$(foreach LIB,$(LIBS),$(LIB).*)

CFLAGS+=-fPIC -Wall $(DFLAGS) \
	$(call PKG-CFLAGS, axiom_user_api) \
	$(AXIOM_COMMON_CFLAGS)

.PHONY: all libs install clean distclean mrproper

all libs: $(foreach lib,$(LIBS),$(lib).$(VERSION))

clean distclean mrproper:
	rm -rf $(CLEANFILES)

libaxiom_init_api.so.$(VERSION): $(OBJS)
	$(CC) -shared -Wl,--soname,libaxiom_init_api.so.$(MAJOR) \
		$(call PKG-LDFLAGS, axiom_user_api) $(AXIOM_COMMON_LDFLAGS) \
		-o $@ $^ \
		$(call PKG-LDLIBS, axiom_user_api) $(AXIOM_COMMON_LDLIBS)

#
# installation
#

install: libs
	mkdir -p $(DESTDIR)$(PREFIX)/lib/pkgconfig ;\
	mkdir -p $(DESTDIR)$(PREFIX)/include/axiom ;\
	for LIB in $(LIBS); do \
		cp $${LIB}.$(VERSION) $(DESTDIR)$(PREFIX)/lib/ ;\
		LIBNAME=$$(basename $$LIB) ;\
		ln -sf $${LIBNAME}.$(VERSION) \
			$(DESTDIR)$(PREFIX)/lib/$${LIBNAME}.$(MAJOR).$(MINOR) ;\
		ln -sf $${LIBNAME}.$(MAJOR).$(MINOR) \
			$(DESTDIR)$(PREFIX)/lib/$${LIBNAME}.$(MAJOR) ;\
		ln -sf $${LIBNAME}.$(MAJOR) \
			$(DESTDIR)$(PREFIX)/lib/$${LIBNAME} ;\
		NAME=$$(basename $$LIB .so | sed -e 's/^lib//') ;\
		D=$$(dirname $$LIB) ;\
		sed -e 's,@PREFIX@,/usr,' \
			$${NAME}.pc.in \
			> $(DESTDIR)$(PREFIX)/lib/pkgconfig/$${NAME}.pc ;\
		cp $(AXIOM_APPS_INCLUDE_DIR)/$${NAME}.h \
			$(DESTDIR)$(PREFIX)/include/axiom ;\
	done
