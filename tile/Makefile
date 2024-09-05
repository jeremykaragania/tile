HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c99 -ffreestanding -Wpedantic -Wall -I.
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -I.
LD = $(HOST)-ld
LDFLAGS = -Ttile.ld
KERNEL_DIR = kernel
LIB_DIR = lib
KERNEL_OBJS = $(addprefix $(KERNEL_DIR)/, asm/helpers.o asm/interrupts.o asm/main.o asm/page.o asm/process.o file.o helpers.o interrupts.o main.o mci.o memory.o page.o process.o uart.o)
LIB_OBJS = $(addprefix $(LIB_DIR)/, string.o)

.PHONY: all
all: tile lib

tile: $(KERNEL_OBJS) $(LIB_OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: lib
lib: $(LIB_OBJS)

.PHONY: clean
clean:
	rm tile $(KERNEL_OBJS) $(LIB_OBJS)
