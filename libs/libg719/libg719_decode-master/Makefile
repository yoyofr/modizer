# G719

export TARGET_OS = $(OS)

CC = gcc
AR = ar
STRIP = strip
RM = rm -f

SRCS = $(wildcard *.c)
SRCS+= $(wildcard reference_code/common/*.c)
SRCS+= $(wildcard reference_code/decoder/*.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))
INCLUDES=-I. -Ireference_code/include

EXTRA_CFLAGS = 
EXTRA_LDFLAGS =
CFLAGS = $(INCLUDES) $(EXTRA_CFLAGS) -O2 -ffast-math -DUSE_ALLOCA -flto
LDFLAGS = -shared -s $(EXTRA_LDFLAGS)
EXTRA_OBJS =

OUTPUT_DIR=.
ifeq ($(TARGET_OS),Windows_NT)
    EXTRA_OBJS = g719.def
    OUTPUT_SHARED = $(OUTPUT_DIR)/libg719_decode.dll
    OUTPUT_STATIC = $(OUTPUT_DIR)/libg719_decode.a
else
    OUTPUT_SHARED = $(OUTPUT_DIR)/libg719_decode.so
    OUTPUT_STATIC = $(OUTPUT_DIR)/libg719_decode.a
endif


all: shared static


#%.o : %.c
#	$(CC) $(INCLUDES) $(CFLAGS) $< -c

static: $(OBJS)
	$(AR) crs $(OUTPUT_STATIC) $(OBJS)

shared: $(OBJS)
	$(CC) $(EXTRA_OBJS) $(OBJS) $(LDFLAGS) -o $(OUTPUT_SHARED)

clean:
	$(RM) $(OBJS) $(OUTPUT_SHARED) $(OUTPUT_STATIC)

.PHONY: all shared static
