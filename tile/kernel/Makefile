HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -ffreestanding -Wpedantic -Wall -I../include/ -I.
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -march=armv7-a
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = asm/main.o asm/process.o main.o memory.o process.o uart.o

kernel: $(OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: clean
clean:
	rm kernel $(OBJS)
