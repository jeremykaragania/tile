HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c99 -ffreestanding -Wpedantic -Wall -I.. -I.
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -I.. -I.
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = asm/helpers.o asm/interrupts.o asm/main.o asm/page.o asm/process.o file.o helpers.o interrupts.o main.o memory.o page.o process.o string.o uart.o

kernel: $(OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: clean
clean:
	rm kernel $(OBJS)
