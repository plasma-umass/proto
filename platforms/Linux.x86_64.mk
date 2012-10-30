
CXXFLAGS += -DCUSTOM_PREFIX\(x\)=proto_\#\#x -msse3
CXXLIB = $(CXX) -shared -fPIC
