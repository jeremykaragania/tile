/*
  pl011.c provides an ARM PrimeCell UART (PL011) driver.
*/

#include <drivers/pl011.h>

/*
  The UART is mapped to this initial address as this is where it is mapped in
  the initial memory map before the proper memory mapping is initialized.
*/
volatile struct uart_registers* uart_0 = (volatile struct uart_registers*)0xffc90000;

struct file_operations uart_operations = {
  .write = uart_write
};


/*
  uart_init initializes the UART.
*/
void uart_init() {
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

/*
  uart_write writes up to "count" bytes from the buffer "buf" to the UART.
*/
int uart_write(int fd, const void* buf, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    uart_putchar(((char*)buf)[i]);
  }

  return count;
}

/*
  uart_putchar writes a byte "c" to the UART data register.
*/
int uart_putchar(const int c) {
  while(uart_0->fr & FR_TXFF);
  uart_0->dr = c;
  return c;
}

/*
  uart_getchar reads a byte from the UART data register. It returns the read
  byte on success or -1 on failure.
*/
int uart_getchar() {
  if (uart_0->fr & FR_RXFE) {
    return -1;
  }
  return uart_0->dr & DR_DATA;
}
