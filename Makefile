
APPS_DIR := axiom-init axiom-run axiom-recv axiom-send axiom-whoami 
APPS_DIR += axiom-ping axiom-traceroute axiom-netperf axiom-rdma axiom-info
APPS_DIR += axiom-utility
LIBS_DIR := axiom-init axiom-run
COMS_DIR := axiom_common_library

.PHONY: all libs com_libs clean install clean distclean 

export DFLAGS

all: libs
	for DIR in $(APPS_DIR); do $(MAKE) -C $$DIR; done

libs: com_libs
	for DIR in $(LIBS_DIR); do $(MAKE) -C $$DIR libs; done

com_libs: 
	for DIR in $(COMS_DIR); do $(MAKE) -C $$DIR; done

install clean distclean:
	for DIR in $(APPS_DIR) $(COMS_DIR); do $(MAKE) -C $$DIR $@; done
