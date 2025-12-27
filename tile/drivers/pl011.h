#ifndef UART_H
#define UART_H

#include <drivers/terminal.h>
#include <kernel/asm/memory.h>
#include <kernel/fifo.h>
#include <kernel/file.h>
#include <stddef.h>
#include <stdint.h>

#define UART_CLK 24000000
#define UART_BAUD_RATE 460800

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

#define IMSC_TXIM (1 << 5)
#define IMSC_RXIM (1 << 4)

#define MIS_TXMIS  (1 << 5)
#define MIS_RXMIS  (1 << 4)

#define ICR_TXIC (1 << 5)
#define ICR_RXIC (1 << 4)

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
  uint32_t ris;
  uint32_t mis;
  uint32_t icr;
  uint32_t dmacr;
};

/*
  struct uart represents a UART. It manages the UART's registers, operations,
  FIFO, and managing terminal.
*/
struct uart {
  volatile struct uart_registers* regs;
  struct uart_operations* ops;
  struct fifo fifo;
  struct terminal* term;
};

/*
  struct uart_operations represents the operations that can be performed on a
  UART.
*/
struct uart_operations {
  int (*read)(struct uart*, void*, size_t);
  int (*write)(struct uart*, const void*, size_t);
};

extern struct uart_operations uart_operations;
extern struct uart uart;
extern struct device uart_device;

void uart_init();

int uart_read(struct uart* u, void* buf, size_t count);
int uart_write(struct uart* u, const void* buf, size_t count);

int uart_putchar(struct uart* u, const int c);
int uart_getchar(struct uart* u);

void uart_begin(struct uart* u);
void uart_end(struct uart* u);

void do_uart_irq_transmit(struct uart* u);
void do_uart_irq_receive(struct uart* u);
void do_uart_irq(struct uart* u);

#endif
