include ../../Makefile.inc

UTIL_MODULE := $(patsubst %.c, %_c.o, $(wildcard *.c))
UTIL_MODULE += $(patsubst %.cc, %_cc.o, $(wildcard *.cc))
UTIL_MODULE += $(patsubst %.cpp, %_cpp.o, $(wildcard *.cpp))
UTIL_MODULE += $(patsubst %.cxx, %_cxx.o, $(wildcard *.cxx))

UTIL_PROJECT_ROOT_DIR := $(subst src/util,,$(CURRENT_DIR))

#这个很重要，否则每次会因为找不到相应的文件而重新编译
VPATH = ../../obj/:../../lib
#下面这两行与上面这行等价
#vpath %.o $(UTIL_PROJECT_ROOT_DIR)/obj/
#vpath %.a $(UTIL_PROJECT_ROOT_DIR)/lib/


UTIL_MODULE_TO_ARCHIVE := $(addprefix $(UTIL_PROJECT_ROOT_DIR)/obj/, $(UTIL_MODULE))

STATIC_LIB_NAME := libutil.a
define compile-cmd
	$(CC) $(CFLAGS) -I$(INC_PATH) -L$(LIB_PATH) -o $(addprefix ../../obj/, $@) -c $<
endef
define archive-cmd
	#$(AR) $(ARFLAGS) -o $(addprefix ../../lib/, $@) $(addprefix ../../obj/, $^)
	$(AR) $(ARFLAGS) -o $(addprefix ../../lib/, $@) $(UTIL_MODULE_TO_ARCHIVE) 
endef
all:$(UTIL_MODULE) $(STATIC_LIB_NAME)
%_c.o:%.c
	$(compile-cmd)
%_cc.o:%.cc
	$(compile-cmd)
%_cpp.o:%.cpp
	$(compile-cmd)
%_cxx.o:%.cxx
	$(compile-cmd)
$(STATIC_LIB_NAME):$(UTIL_MODULE)
	$(archive-cmd)
.phony:clean
clean:
	rm -rf ../../lib/$(STATIC_LIB_NAME)

