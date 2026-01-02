HOST = arm-none-eabi
CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -I.
AS = as
ASFLAGS = -I.
LD = ld

tile.ld: CFLAGS := $(CFLAGS) -E -x c -P
kernel drivers lib user: CC := $(HOST)-$(CC)
kernel drivers lib: CFLAGS := $(CFLAGS) -ffreestanding
kernel drivers lib: AS := $(HOST)-$(AS)
kernel drivers lib: ASFLAGS := $(ASFLAGS) -mcpu=cortex-a15
tile kernel drivers lib: LD := $(HOST)-$(LD)
tile kernel drivers lib: LDFLAGS := $(LDFLAGS) -Ttile.ld
tools: CFLAGS := $(CFLAGS)
user: CFLAGS := $(CFLAGS) -static -ffreestanding -nostdlib -emain

KERNEL_DIR = kernel
DRIVERS_DIR = drivers
LIB_DIR = lib
TOOLS_DIR = tools
USER_DIR = user

BUILD_DIR = build
TOOLS_BUILD_DIR = $(BUILD_DIR)/$(TOOLS_DIR)
USER_BUILD_DIR = $(BUILD_DIR)/$(USER_DIR)

KERNEL_OBJS = $(addprefix $(KERNEL_DIR)/, asm/helpers.o asm/interrupts.o asm/main.o asm/page.o asm/process.o asm/processor.o asm/schedule.o asm/syscall.o buffer.o device.o fifo.o file.o helpers.o interrupts.o list.o log.o main.o memory.o page.o process.o processor.o schedule.o syscall.o)
DRIVERS_OBJS = $(addprefix $(DRIVERS_DIR)/, gic_400.o pl011.o pl180.o sp804.o terminal.o)
LIB_OBJS = $(addprefix $(LIB_DIR)/, asm/syscall.o elf.o stdlib.o string.o)
TOOLS_OBJS = $(addprefix $(TOOLS_BUILD_DIR)/, mkfs)
USER_OBJS = $(addprefix $(USER_BUILD_DIR)/, init)

.PHONY: all
all: tile tools user

tile: tile.ld kernel drivers lib | $(BUILD_DIR)
	$(LD) $(KERNEL_OBJS) $(DRIVERS_OBJS) $(LIB_OBJS) -o$(BUILD_DIR)/tile $(LDFLAGS)

tile.ld:
	$(CC) $(CFLAGS) tile.ld.S -o$@

.PHONY: kernel
kernel: $(KERNEL_OBJS)

.PHONY: drivers
drivers: $(DRIVERS_OBJS)

.PHONY: lib
lib: $(LIB_OBJS)

.PHONY: tools
tools: $(TOOLS_OBJS)

$(TOOLS_BUILD_DIR)/%: $(TOOLS_DIR)/%/main.c | $(TOOLS_BUILD_DIR)
	$(CC) $^ -o$@ $(CFLAGS)

.PHONY: user
user: $(USER_OBJS)

$(USER_BUILD_DIR)/%: $(USER_DIR)/%/main.c | $(USER_BUILD_DIR) lib
	$(CC) $^ $(LIB_OBJS) -o$@ $(CFLAGS)

.SECONDARY: $(BUILD_DIR)
$(BUILD_DIR):
	mkdir $(BUILD_DIR)

.SECONDARY: $(TOOLS_BUILD_DIR)
$(TOOLS_BUILD_DIR): | $(BUILD_DIR)
	mkdir $(TOOLS_BUILD_DIR)

.SECONDARY: $(USER_BUILD_DIR)
$(USER_BUILD_DIR): | $(BUILD_DIR)
	mkdir $(USER_BUILD_DIR)

.PHONY: clean
clean:
	rm -r tile.ld $(KERNEL_OBJS) $(DRIVERS_OBJS) $(LIB_OBJS) $(BUILD_DIR)
