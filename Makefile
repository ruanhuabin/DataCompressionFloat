include Makefile.inc

BIN_DIR := bin
OBJ_DIR := obj 
UTIL_TARGET := util_tool
CORE_TARGET := core_tool
TOOL_TARGET := build_tool
MAIN_TARGET := main

#$(BIN_DIR)不能直接用字符串代替，否则每次make的时候都会执行字符串对应的规则，即使bin目录已经存在了
all:$(BIN_DIR) $(OBJ_DIR) $(UTIL_TARGET) $(CORE_TARGET) $(TOOL_TARGET) $(MAIN_TARGET) 
vpath % bin
$(BIN_DIR):
	if [ ! -d "./$(BIN_DIR)" ]; then \
		mkdir $(BIN_DIR);\
	fi
$(OBJ_DIR):
	if [ ! -d "./$(OBJ_DIR)" ]; then \
		mkdir $(OBJ_DIR);\
	fi
$(UTIL_TARGET):
	cd src/util && make
$(CORE_TARGET):
	cd src/core && make
$(TOOL_TARGET):
	cd src/tool && make
$(MAIN_TARGET):
	cd src/main && make
.phony:clean
clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR) lib/libcore.a lib/libutil.a 
