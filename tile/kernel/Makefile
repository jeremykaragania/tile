HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -ffreestanding -Wpedantic -Wall -I.. -I.
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = asm/main.o asm/memory.o asm/process.o interrupts.o main.o memory.o process.o string.o uart.o

kernel: $(OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: clean
clean:
	rm kernel $(OBJS)
