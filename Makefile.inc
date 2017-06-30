#这个值会根据当前路径的变化而变化
CURRENT_DIR := $(shell pwd)
LIB_PATH := ${CURRENT_DIR}/../../lib/
INC_PATH := ${CURRENT_DIR}/../../src/include/
BIN_PATH := ${CURRENT_DIR}/bin

MODULE_DIR_NAME := $(shell ls $(CURRENT_DIR)/src/ 2>/dev/null)
MODULE_LIBS_1 := $(subst main,,$(MODULE_DIR_NAME))
MODULE_LIBS_2 = $(subst tool,,$(MODULE_LIBS_1))
MODULE_LIBS_3 = $(subst include,,$(MODULE_LIBS_2))
MODULE_LIBS_4 = $(subst util,,$(MODULE_LIBS_3))
MODULE_LIBS_5 = $(subst core,,$(MODULE_LIBS_4))

FINAL_MODULE_LIBS := $(addprefix -l, $(MODULE_LIBS_5))
CC := gcc
CC_CPP:= g++
AR := ar
OPT_LEVEL = -O0 
DEBUG_FLAG = -g
LIBS := -lutil -lcore -lz -lpthread -lm
#LIBS := -lutil -lcore $(FINAL_MODULE_LIBS)
CFLAGS := -Wall -std=gnu99 ${OPT_LEVEL} ${DEBUG_FLAG}
CFLAGS_CPP := -Wall ${OPT_LEVEL} ${DEBUG_FLAG}
ARFLAGS := -rcs 

