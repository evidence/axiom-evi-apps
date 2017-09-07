
APPS_DIR := axiom-init axiom-run axiom-recv axiom-send axiom-whoami
APPS_DIR += axiom-ping axiom-traceroute axiom-netperf axiom-rdma axiom-info
APPS_DIR += axiom-utility axiom-ethtap axiom-rdma-dbg
LIBS_DIR := axiom-init axiom-run
COMS_DIR := axiom_common_library
TESTS_DIR := tests

.PHONY: all libs com_libs clean install clean distclean tests

export DFLAGS

all: libs
	for DIR in $(APPS_DIR); do { $(MAKE) -C $$DIR || exit 1; }; done

libs: com_libs
	for DIR in $(LIBS_DIR); do { $(MAKE) -C $$DIR libs || exit 1; }; done

tests: all
	for DIR in $(TESTS_DIR); do { $(MAKE) -C $$DIR install || exit 1; }; done

com_libs:
	for DIR in $(COMS_DIR); do { $(MAKE) -C $$DIR || exit 1; }; done

install:
	for DIR in $(APPS_DIR) $(COMS_DIR); do { $(MAKE) -C $$DIR $@ || exit 1; }; done

clean distclean:
	for DIR in $(APPS_DIR) $(COMS_DIR) $(TESTS_DIR); do { $(MAKE) -C $$DIR $@ || exit 1; }; done
