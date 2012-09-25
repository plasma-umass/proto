
CXXFLAGS += -D'CUSTOM_PREFIX(x)=proto\#\#x' -msse3
CXXLIB = $(CXX) -shared -fPIC
