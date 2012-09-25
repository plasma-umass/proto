ROOT = .
DIRS = src

include $(ROOT)/common.mk

test: build
	$(MAKE) -C tests test

