#include <uart.h>

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

volatile struct uart_registers* uart_0 = (volatile struct uart_registers*)0x1c090000;

void uart_init() {
  uint32_t lcr_h = 0;
  uart_0->cr &= ~CR_UARTEN;
  while (uart_0->fr & FR_BUSY);
  uart_0->lcr_h &= ~LCR_H_FEN;
  uart_0->ibrd = 3;
  uart_0->fbrd = 16;
  uart_0->lcr_h |= LCR_H_FEN;
  uart_0->lcr_h |= LCR_H_WLEN;
  uart_0->lcr_h &= ~LCR_H_STP2;
  uart_0->cr |= CR_UARTEN;
}

int uart_putchar(const int c) {
  while(uart_0->fr & FR_TXFF);
  uart_0->dr = c;
  return c;
}

int uart_puts(const char* s) {
  size_t i = 0;
  while (s[i]) {
    uart_putchar(s[i]);
    ++i;
  }
  uart_putchar('\n');
  return 0;
}

int uart_getchar() {
  if (uart_0->fr & FR_RXFE) {
    return -1;
  }
  return uart_0->dr & DR_DATA;
}
