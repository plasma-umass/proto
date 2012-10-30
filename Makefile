ROOT = .
DIRS = src

include $(ROOT)/common.mk

clean::
	@$(MAKE) -C tests clean

test: build
	@$(MAKE) -C tests test

