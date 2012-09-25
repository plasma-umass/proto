ROOT = ../..
TARGETS = runner
LIBS = gtest proto

include $(ROOT)/common.mk

test: build
	@LD_LIBRARY_PATH=$(ROOT) ./runner --gtest_output=xml

