HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -I../include/ -I. -ffreestanding
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -march=armv7-a
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = kernel console.o head.o main.o memory.o

all: $(OBJS)

kernel: console.o head.o main.o memory.o
	$(LD) $^ -o$@ $(LDFLAGS)

console.o:

head.o:

main.o:

memory.o:

.PHONY: clean
clean:
	rm *.o
