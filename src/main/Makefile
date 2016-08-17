include ../../Makefile.inc
BIN_MODULE := $(patsubst %.c, %_c, $(wildcard *.c))
BIN_MODULE += $(patsubst %.cc, %_cc, $(wildcard *.cc))
BIN_MODULE += $(patsubst %.cpp, %_cpp, $(wildcard *.cpp))
BIN_MODULE += $(patsubst %.cxx, %_cxx, $(wildcard *.cxx))

define link-cmd
	@rm -rf $(BIN_MODULE)
	$(CC) $(CFLAGS) -I$(INC_PATH) -L$(LIB_PATH) -o $(addprefix ../../bin/, $@) $< $(LIBS)
endef
VPATH = ../../bin
all:$(BIN_MODULE)
%_c:%.c
	$(link-cmd)
%_cc:%.cc
	$(link-cmd)
%_cpp:%.cpp
	$(link-cmd)
%_cxx:%.cxx
	$(link-cmd)
.phony:clean
clean:
	rm -rf $(BIN_MODULE)

