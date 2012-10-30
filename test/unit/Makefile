
BITS=32

GTEST_DIR = $(PROTO_LIB_DIR)/test/unit/gtest-1.6.0

GTEST_SRCS = $(GTEST_DIR)/src/gtest-all.cc $(GTEST_DIR)/src/gtest_main.cc

SRCS = $(GTEST_SRCS) test.cc

GTEST_FLAGS = -DGTEST_HAS_PTHREAD=1

NAME = gtest

CXX = g++
CXX_FLAGS = -m$(BITS) $(GTEST_FLAGS) -I$(GTEST_DIR) -I$(GTEST_DIR)/include -O0 -Wall -Wextra -g

PTHREAD_FLAGS = -lpthread
PROTO_FLAGS = -rdynamic ${PROTO_LIB_DIR}/libproto.so

all: $(SRCS) $(DEPS)
	$(CXX) -o $(NAME)_ref $(SRCS) $(CXX_FLAGS) $(PTHREAD_FLAGS)
	$(CXX) -o $(NAME)_proto $(SRCS) $(CXX_FLAGS) $(PROTO_FLAGS)

clean:
	rm -f $(NAME)_ref $(NAME)_proto
