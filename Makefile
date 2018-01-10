
include configure.mk

APPS_DIR := axiom-init axiom-run axiom-recv axiom-send axiom-whoami
APPS_DIR += axiom-ping axiom-traceroute axiom-netperf axiom-rdma axiom-info
APPS_DIR += axiom-utility axiom-ethtap axiom-rdma-dbg
LIBS_DIR := axiom-init axiom-run
#LIBS_DIR_EXTRA are LIBS_DIR than are not APPS_DIR
LIBS_DIR_EXTRA :=
COMS_DIR := axiom_common_library
TESTS_DIR := tests

.PHONY: all \
	$(APPS_DIR) $(LIBS_DIR) $(COMS_DIR) $(TEST_DIR) \
	libs libs-install \
	tests tests-install tests-clean \
	install clean distclean mrproper

.DEFAULT_GOAL := all
all: $(APPS_DIR) $(LIBS_DIR_EXTRA) $(COMS_DIR)

configure: configure-save

install: 
	for DIR in $(APPS_DIR) $(LIBS_DIR_EXTRA);\
		do { $(MAKE) -C $$DIR $@ || exit 1; };\
	done

clean distclean mrproper:
	for DIR in $(APPS_DIR) $(COMS_DIR) $(LIBS_DIR_EXTRA) $(TESTS_DIR);\
		do { $(MAKE) -C $$DIR $@ || exit 1; };\
	done

$(COMS_DIR):
	$(MAKE) -C $@

$(APPS_DIR) $(LIBS_DIR_EXTRA): $(COMS_DIR)
	$(MAKE) -C $@

libs libs-install: $(COMS_DIR)

libs libs-install libs-clean libs-distclean libs-mrproper:
	for DIR in $(LIBS_DIR);\
		do { $(MAKE) -C $$DIR $@ || exit 1; };\
	done

tests tests-install tests-clean tests-distclean tests-mrproper: tests
	for DIR in $(TESTS_DIR);\
		do { $(MAKE) -C $$DIR $@ || exit 1; };\
	done
