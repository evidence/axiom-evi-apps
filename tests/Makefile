
export KERN FS

include ../common.mk

.PHONY: clean distclean build install

SUBDIRS=$(wildcard test*)

install build clean distclean:
	make -C common $@
	for dir in $(SUBDIRS); do { $(MAKE) -C $$dir $@ || exit 1; }; done
