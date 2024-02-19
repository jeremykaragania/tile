HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -I../include/ -I. -ffreestanding
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -march=armv7-a
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = console.o asm/main.o main.o memory.o

kernel: $(OBJS)
	$(LD) $^ -o$@ $(LDFLAGS)

.PHONY: clean
clean:
	rm *.o
	rm asm/*.o
