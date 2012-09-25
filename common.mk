# Get the current OS and architecture
OS ?= $(shell uname -s)
CPU ?= $(shell uname -m)

# Set the default compilers and flags
CC = gcc
CXX = g++
CFLAGS ?= -O3 -g -finline-limit=20000 -L$(ROOT)
CXXFLAGS ?= $(CFLAGS)

# Include platform-specific rules
include $(ROOT)/platforms/$(OS).$(CPU).mk

# Set the default shared library filename suffix
SHLIB_SUFFIX ?= so

# Don't build into subdirectories by default
DIRS ?=

# Don't require any libraries by default
LIBS ?= 

# Set the default include directories
INCLUDE_DIRS ?= $(ROOT)/include $(ROOT)/heaplayers $(ROOT)/heaplayers/util

# Recurse into subdirectories for the 'clean' and 'build' targets
RECURSIVE_TARGETS ?= clean build

# Build by default
all: build

# Just remove the targets
clean::
ifneq ($(TARGETS),)
	@rm -f $(TARGETS)
endif

# Set the default source and include files with wildcards
SRCS ?= $(wildcard *.c) $(wildcard *.cpp) $(wildcard *.cc) $(wildcard *.C)
INCLUDES ?= $(wildcard *.h) $(wildcard *.hpp) $(wildcard *.hh) $(wildcard *.H) $(wildcard $(addsuffix /*.h, $(INCLUDE_DIRS))) $(wildcard $(addsuffix /*.hpp, $(INCLUDE_DIRS))) $(wildcard $(addsuffix /*.hh, $(INCLUDE_DIRS))) $(wildcard $(addsuffix /*.H, $(INCLUDE_DIRS)))

# Generate flags to link required libraries and get includes
LIBFLAGS = $(addprefix -l, $(LIBS))
INCFLAGS = $(addprefix -I, $(INCLUDE_DIRS))

build:: $(TARGETS)

$(filter %.$(SHLIB_SUFFIX), $(TARGETS)):: $(SRCS) $(INCLUDES)
	$(CXXLIB) $(CXXFLAGS) $(INCFLAGS) $(SRCS) -o $@ $(LIBFLAGS)

$(filter-out %.$(SHLIB_SUFFIX), $(TARGETS)):: $(SRCS) $(INCLUDES)
	$(CXX) $(CXXFLAGS) $(INCFLAGS) $(SRCS) -o $@ $(LIBFLAGS)

$(RECURSIVE_TARGETS)::
	@for dir in $(DIRS); do \
	  $(MAKE) -C $$dir $@; \
	done
