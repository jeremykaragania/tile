#ifndef UART_H
#define UART_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

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

void uart_init();

int uart_putchar(const int c);

void uart_putint(const int a);

void uart_putuint(unsigned int a);

void uart_puthex(unsigned int a);

int uart_puts(const char* s);

int uart_printf(const char *format, ...);

int uart_getchar();

#endif
