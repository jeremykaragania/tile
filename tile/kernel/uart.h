#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define UART_CLK 24000000
#define BAUD_RATE 460800
#define DR_DATA 255 << 0
#define FR_TXFE 1 << 7
#define FR_RXFF 1 << 6
#define FR_TXFF 1 << 5
#define FR_RXFE 1 << 4
#define FR_BUSY 1 << 3
#define LCR_H_WLEN 3 << 5
#define LCR_H_FEN 1 << 4
#define LCR_H_STP2 1 << 3
#define CR_RXE 1 << 9
#define CR_TXE 1 << 8
#define CR_UARTEN 1 << 0

struct uart_registers {
  uint32_t dr;
  uint32_t rsr_ecr;
  uint32_t reserved_0[4];
  const uint32_t fr;
  uint32_t reserved_1;
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

enum length_modifier {
  LM_NONE,
  LM_CHAR,
  LM_SHORT,
  LM_LONG,
  LM_LONG_LONG
};

extern volatile struct uart_registers* uart_0;

void uart_init();

void uart_put_signed_integer(long long a);
void uart_put_unsigned_integer(unsigned long long a, const char format);

int uart_putchar(const int c);
int uart_putstring(const char* s);
int uart_puts(const char* s);

int uart_printf(const char *format, ...);

int uart_getchar();

#endif
