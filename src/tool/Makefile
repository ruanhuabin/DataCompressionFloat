include ../../Makefile.inc
BIN_MODULE := $(patsubst %.c, %_c, $(wildcard *.c))
BIN_MODULE += $(patsubst %.cc, %_cc, $(wildcard *.cc))
BIN_MODULE += $(patsubst %.cpp, %_cpp, $(wildcard *.cpp))
BIN_MODULE += $(patsubst %.cxx, %_cxx, $(wildcard *.cxx))

TOOL_LIBS := -lutil -lz -lpthread -lm

define link-cmd
	$(CC) $(CFLAGS) -I$(INC_PATH) -L$(LIB_PATH) -o $(addprefix ../../bin/, $@) $< $(TOOL_LIBS)
endef

 define link-cmd-cpp
	$(CC_CPP) $(CFLAGS_CPP) -I$(INC_PATH) -L$(LIB_PATH) -o $(addprefix ../../bin/, $@) $< $(TOOL_LIBS)
endef


VPATH = ../../bin
all:$(BIN_MODULE)
%_c:%.c
	$(link-cmd)
%_cc:%.cc
	$(link-cmd)
%_cpp:%.cpp
	$(link-cmd-cpp)
%_cxx:%.cxx
	$(link-cmd)
.phony:clean
clean:
	rm -rf $(BIN_MODULE)

