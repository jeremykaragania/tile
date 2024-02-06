HOST = arm-none-eabi
CC = $(HOST)-gcc
CFLAGS = -std=c89 -ffreestanding
AS = $(HOST)-as
ASFLAGS = -mcpu=cortex-a15 -march=armv7-a
LD = $(HOST)-ld
LDFLAGS = -Tkernel.ld
OBJS = kernel head.o main.o

all: $(OBJS)

kernel: head.o main.o
	$(LD) $^ -o$@ $(LDFLAGS)

head.o:

main.o:

clean:
	rm *.o
