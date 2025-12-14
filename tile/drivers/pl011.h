#ifndef UART_H
#define UART_H

#include <kernel/asm/memory.h>
#include <kernel/device.h>
#include <kernel/fifo.h>
#include <kernel/file.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define UART_CLK 24000000
#define BAUD_RATE 460800
#define DR_DATA (255 << 0)
#define FR_TXFE (1 << 7)
#define FR_RXFF (1 << 6)
#define FR_TXFF (1 << 5)
#define FR_RXFE (1 << 4)
#define FR_BUSY (1 << 3)
#define LCR_H_WLEN (3 << 5)
#define LCR_H_FEN (1 << 4)
#define LCR_H_STP2 (1 << 3)
#define CR_RXE (1 << 9)
#define CR_TXE (1 << 8)
#define CR_UARTEN (1 << 0)
#define UART_FIFO_SIZE 256

/*
  struct uart_registers represents the registers of the PrimeCell UART (PL011).
*/
struct uart_registers {
  uint32_t dr;
  uint32_t rsr_ecr;
  const uint32_t reserved_0[4];
  const uint32_t fr;
  const uint32_t reserved_1;
  uint32_t ilpr;
  uint32_t ibrd;
  uint32_t fbrd;
  uint32_t lcr_h;
  uint32_t cr;
  uint32_t ifls;
  uint32_t imsc;
  uint32_t is;
  uint32_t mis;
  uint32_t icr;
  uint32_t dmacr;
};

extern volatile struct uart_registers* uart_0;
extern struct file_operations uart_operations;
extern struct device uart_device;
extern struct fifo uart_fifo;

void uart_init();

int uart_read(int fd, void* buf, size_t count);
int uart_write(int fd, const void* buf, size_t count);

int uart_putchar(const int c);
int uart_getchar();

#endif
