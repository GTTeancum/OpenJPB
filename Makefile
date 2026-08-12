XBE_TITLE = Jedi\ Power\ Battles\ Reconstruction
OUTPUT_DIR = build/xbox

SRCS = \
	src/port/xbox_main.c \
	src/reconstructed/original/list.c \
	src/reconstructed/original/timer.c \
	src/reconstructed/original/alloc.c \
	src/reconstructed/original/memory.c \
	src/reconstructed/original/fmath.c \
	src/reconstructed/original/vectors.c

JPB_OBJS = $(SRCS:.c=.obj)

# Keep project language/warning policy local to project objects. nxdk's build
# graph also exposes its library objects through this Makefile, and its HAL
# sources intentionally use GNU extensions such as `asm`.
$(JPB_OBJS): NXDK_CFLAGS += -Iinclude -std=c11 -Wall -Wextra

NXDK_DIR ?= /c/nxdk
include $(NXDK_DIR)/Makefile
