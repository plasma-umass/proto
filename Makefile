SRCS = libproto.cpp dlmalloc.c realfuncs.cpp gnuwrapper.cpp process.cpp xthread.cpp xscheduler.cpp xprotect.cpp
DEPS = $(SRCS) xprotect.h xdefines.h xglobals.h xprotect.h xplock.h xrun.h

# CXX = icc
CXX = g++

INCLUDE_DIRS = -I. -I./heaplayers -I./heaplayers/util

MACOS_COMPILE := $(CXX) -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) -compatibility_version 1 -current_version 1 -dynamiclib -o libproto.dylib -ldl -lpthread

MACOS_COMPILE_DEBUG := $(CXX) -g -O0 -m32 $(INCLUDE_DIRS) -shared -fPIC $(SRCS)  -compatibility_version 1 -current_version 1 -dynamiclib -o libproto.dylib -lpthread -ldl

all: gcc-x86-debug
	@echo "Specify the desired build target."
	@echo "  macos"
	@echo "  gcc-x86"
	@echo "  gcc-x86-debug"
	@echo "  gcc-x86-64"
	@echo "  gcc-x86-64-debug"
	@echo "  gcc-sparc"

.PHONY: gcc-x86 gcc-x86-debug gcc-x86-64 gcc-x86-64-debug gcc-sparc clean

macos: $(SRCS) $(DEPS)
	$(MACOS_COMPILE)

macos-debug: $(SRCS) $(DEPS)
	$(MACOS_COMPILE_DEBUG)

gcc-x86: $(SRCS) $(DEPS)
	gcc -m32 -c memdiff.S
	$(CXX) $(SRCS) $(INCLUDE_DIRS) -DNDEBUG -shared -m32 -msse3 -fPIC -D'CUSTOM_PREFIX(x)=proto##x' -o libproto.so -ldl -lpthread -O3 -finline-limit=20000 

gcc-x86-debug: $(SRCS) $(DEPS)
#	gcc -m32 -gdwarf-2 -fno-dwarf2-cfi-asm -o lowlevel.o lowlevellock.S
#	gcc -m32 -Wa,-alh,-L -funwind-tables -fasynchronous-unwind-tables -o lowlevel.o lowlevellock.S
#	gcc -m32 -march=core2 -c lowlevellock.S
	$(CXX) -m32 -DDEBUG_LEVEL=3 -g -O0 -fno-inline $(INCLUDE_DIRS) -shared -fPIC -Wno-invalid-offsetof -D_GNU_SOURCE -D'CUSTOM_PREFIX(x)=proto_##x' $(SRCS) -o libproto.so  -ldl -lpthread

gcc-x86-64: $(SRCS) $(DEPS)
	$(CXX) -DNDEBUG -O3 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) -o libproto.so  -ldl -lpthread

gcc-x86-64-debug: $(SRCS) $(DEPS)
	$(CXX) $(INCLUDE_DIRS) -shared -fPIC -g  $(SRCS) -o libproto.so  -ldl -lpthread

gcc-sparc: $(SRCS) $(DEPS)
	g++ -g -m32 $(INCLUDE_DIRS) -shared -fPIC $(SRCS) -o libproto.so  -ldl -lrt -lpthread

#gcc-sparc: $(SRCS) $(DEPS)
#	g++ -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) -o libproto.so  -ldl -lpthread

test: all
	python3 run_tests.py itest
	python3 run_tests.py test

clean:
	rm -f libproto.so

