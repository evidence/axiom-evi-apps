
.PHONY: clean build install

SUBDIRS=$(wildcard test*)

build clean install:
	make -C common $@
	for dir in $(SUBDIRS); do make -C $$dir $@; done
