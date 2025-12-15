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
  .read = uart_read,
  .write = uart_write
};

struct device uart_device = {
  .name = "console",
  .ops = &uart_operations,
  .major = 5,
  .minor = 1,
  .type = DT_CHARACTER
};

struct fifo uart_fifo;

/*
  uart_init initializes the UART.
*/
void uart_init() {
  /* Disable the UART. */
  uart_0->cr &= ~CR_UARTEN;

  /* Wait for the end of transmission or reception of the current character. */
  while (uart_0->fr & FR_BUSY);

  uart_0->lcr_h &= ~LCR_H_FEN;

  /*
    The baud rate divisor is represented as a 22-bit number with a 16-bit
    integer and 6-bit fractional part. The below values are for a baud rate of
    460800.
  */
  uart_0->ibrd = 3;
  uart_0->fbrd = 16;

  uart_0->lcr_h |= LCR_H_FEN;
  uart_0->lcr_h |= LCR_H_WLEN;
  uart_0->lcr_h &= ~LCR_H_STP2;

  /* Enable the UART. */
  uart_0->cr |= CR_UARTEN;

  fifo_alloc(&uart_fifo, UART_FIFO_SIZE, 1);
}

/*
  uart_read reads up to "count" bytes into the buffer "buf" from the UART.
*/
int uart_read(int fd, void* buf, size_t count) {
  int c;

  for (size_t i = 0; i < count; ++i) {
    c = -1;

    while (c < 0) {
      c = uart_getchar();
    }

    ((char*)buf)[i] = c;
  }

  return count;
}

/*
  uart_write writes up to "count" bytes from the buffer "buf" to the UART.
*/
int uart_write(int fd, const void* buf, size_t count) {
  int ret;

  ret = fifo_push_n(&uart_fifo, (char*)buf + 1, count - 1) + 1;

  /* We need to kick off the transmit interrupt by writing to the FIFO. */
  uart_0->dr = *(char*)buf;
  uart_begin();

  return ret;
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

/*
  uart_begin begins UART transmission.
*/
void uart_begin() {
  uart_0->imsc |= IMSC_TXIM;
}

/*
  uart_end ends UART transmission.
*/
void uart_end() {
  uart_0->imsc &= ~IMSC_TXIM;
}

/*
  do_uart_irq_transmit handles a UART transmit IRQ exception.
*/
void do_uart_irq_transmit() {
  char c;

  while (1) {
    if (uart_0->fr & FR_TXFF) {
      break;
    }

    if (fifo_pop(&uart_fifo, &c) < 0) {
      break;
    }

    uart_0->dr = c;
  }

  if (is_fifo_empty(&uart_fifo)) {
    uart_end();
  }
}

/*
  do_uart_irq handles a UART IRQ exception.
*/
void do_uart_irq() {
  uint32_t mis = uart_0->mis;

  if (mis & MIS_TXMIS) {
    do_uart_irq_transmit();
  }
}
