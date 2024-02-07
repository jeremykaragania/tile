HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -I../include/ -I. -ffreestanding
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -march=armv7-a
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = kernel head.o main.o memory.o

all: $(OBJS)

kernel: head.o main.o memory.o
	$(LD) $^ -o$@ $(LDFLAGS)

head.o:

main.o:

memory.o:

.PHONY: clean
clean:
	rm *.o
