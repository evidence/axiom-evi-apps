
include ../common.mk

.PHONY: clean distclean mrproper build install

SUBDIRS=$(wildcard test*)

install build clean distclean mrproper:
	make -C common $@
	for dir in $(SUBDIRS); do { $(MAKE) -C $$dir $@ || exit 1; }; done
