HOST = arm-none-eabi
CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -I.
AS = as
ASFLAGS = -I.
LD = ld

tile lib: CC := $(HOST)-$(CC)
tile lib: CFLAGS := $(CFLAGS) -ffreestanding
tile lib: AS := $(HOST)-$(AS)
tile lib: ASFLAGS := $(ASFLAGS) -mcpu=cortex-a15
tile lib: LD := $(HOST)-$(LD)
tile lib: LDFLAGS := $(LDFLAGS) -Ttile.ld

%: CFLAGS := $(CFLAGS)

KERNEL_DIR = kernel
DRIVERS_DIR = drivers
LIB_DIR = lib
TOOLS_DIR = tools
KERNEL_OBJS = $(addprefix $(KERNEL_DIR)/, asm/clock.o asm/helpers.o asm/interrupts.o asm/main.o asm/page.o asm/process.o asm/processor.o buffer.o clock.o file.o helpers.o interrupts.o main.o memory.o page.o process.o processor.o)
DRIVERS_OBJS = $(addprefix $(DRIVERS_DIR)/, pl011.o pl180.o)
LIB_OBJS = $(addprefix $(LIB_DIR)/, string.o)
TOOLS_OBJS = mkfs

.PHONY: all
all: tile drivers lib tools

tile: $(KERNEL_OBJS) $(DRIVERS_OBJS) $(LIB_OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: drivers
drivers: $(DRIVERS_OBJS)

.PHONY: lib
lib: $(LIB_OBJS)

.PHONY: tools
tools: $(TOOLS_OBJS)

%: $(TOOLS_DIR)/%/main.c
	$(CC) $^ -o$@ $(CFLAGS)

.PHONY: clean
clean:
	rm tile $(KERNEL_OBJS) $(DRIVERS_OBJS) $(LIB_OBJS) $(TOOLS_OBJS)
