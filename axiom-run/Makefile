
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
	for DESTDIR in $(SYSROOT_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/lib/pkgconfig ;\
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
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/lib ;\
		for LIB in $(LIBS); do \
			cp $${LIB}.$(VERSION) $${DESTDIR}/usr/lib/ ;\
			LIBNAME=$$(basename $$LIB) ;\
			ln -sf $${LIBNAME}.$(VERSION) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR).$(MINOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR).$(MINOR) $${DESTDIR}/usr/lib/$${LIBNAME}.$(MAJOR) ;\
			ln -sf $${LIBNAME}.$(MAJOR) $${DESTDIR}/usr/lib/$${LIBNAME} ;\
		done ;\
	done

#test/myinfo: test/myinfo.o
#	$(CC) -L$(PWD)/lib -o test/myinfo test/myinfo.o -laxiom_run_api
#
#test/myexch: test/myexch.o
#	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -o test/myexch test/myexch.o -laxiom_user_api -lpthread
#
#test/myrdma: test/myrdma.o
#	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -L$(PWD)/$(AXIOM_OVERLAY)/usr/lib -Wl,-T$(AXIOM_ALLOC_LDS) -o test/myrdma test/myrdma.o -laxiom_allocator -levi_lmm -laxiom_user_api -lpthread
#
#test/mybar: test/mybar.o lib/libaxiom_run_api.a
#	$(CC) -L$(PWD)/lib -o test/mybar test/mybar.o -laxiom_run_api
#
#test/myrpc: test/myrpc.o lib/libaxiom_run_api.a
#	$(CC) -L$(PWD)/lib -o test/myrpc test/myrpc.o -laxiom_run_api
#
#test/myputbulk: test/myputbulk.o
#	$(CC) -L$(PWD)/../../axiom-evi-nic/axiom_user_library -o test/myputbulk test/myputbulk.o -laxiom_user_api
#

install: all libs
	for DESTDIR in $(TARGET_INST_DIR); do \
		mkdir -p $${DESTDIR}/usr/bin ;\
		cp $(APPS) $${DESTDIR}/usr/bin/ ;\
	done

clean distclean:
	rm -rf $(CLEANFILES)
