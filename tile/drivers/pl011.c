/*
  pl011.c provides an ARM PrimeCell UART (PL011) driver.
*/

#include <drivers/pl011.h>

struct file_operations uart_operations = {
  .read = uart_read,
  .write = uart_write
};

struct uart uart = {
  /*
    The UART's registters are mapped to this initial address as this is where
    it is mapped in the initial memory map before the proper memory mapping is
    initialized.
  */
  .regs = (volatile struct uart_registers*)0xffc90000,
  .ops = &uart_operations,
  .term = &terminal
};

/*
  uart_init initializes the UART.
*/
void uart_init() {
  /* Disable the UART. */
  uart.regs->cr &= ~CR_UARTEN;

  /* Wait for the end of transmission or reception of the current character. */
  while (uart.regs->fr & FR_BUSY);

  uart.regs->lcr_h &= ~LCR_H_FEN;

  /*
    The baud rate divisor is represented as a 22-bit number with a 16-bit
    integer and 6-bit fractional part. The below values are for a baud rate of
    460800.
  */
  uart.regs->ibrd = 3;
  uart.regs->fbrd = 16;

  uart.regs->lcr_h |= LCR_H_FEN;
  uart.regs->lcr_h |= LCR_H_WLEN;
  uart.regs->lcr_h &= ~LCR_H_STP2;

  /* Enable the UART. */
  uart.regs->cr |= CR_UARTEN;

  fifo_alloc(&uart.fifo, UART_FIFO_SIZE, 1);
}

/*
  uart_read reads up to "count" bytes into the buffer "buf" from the UART.
*/
int uart_read(struct file_info_int* file, void* buf, size_t count) {
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
int uart_write(struct file_info_int* file, const void* buf, size_t count) {
  int ret;

  struct terminal* term = file_to_terminal(file);

  ret = fifo_push_n(&uart.fifo, buf, count);

  uart_begin();
  do_uart_irq_transmit();

  return ret;
}

/*
  uart_putchar writes a byte "c" to the UART data register.
*/
int uart_putchar(const int c) {
  while(uart.regs->fr & FR_TXFF);
  uart.regs->dr = c;
  return c;
}

/*
  uart_getchar reads a byte from the UART data register. It returns the read
  byte on success or -1 on failure.
*/
int uart_getchar() {
  if (uart.regs->fr & FR_RXFE) {
    return -1;
  }
  return uart.regs->dr & DR_DATA;
}

/*
  uart_begin begins UART transmission.
*/
void uart_begin() {
  uart.regs->imsc |= IMSC_TXIM;
}

/*
  uart_end ends UART transmission.
*/
void uart_end() {
  uart.regs->imsc &= ~IMSC_TXIM;
}

/*
  do_uart_irq_transmit handles a UART transmit IRQ exception.
*/
void do_uart_irq_transmit() {
  char c;

  while (1) {
    if (uart.regs->fr & FR_TXFF) {
      break;
    }

    if (fifo_pop(&uart.fifo, &c) < 0) {
      break;
    }

    uart.regs->dr = c;
  }

  if (is_fifo_empty(&uart.fifo)) {
    uart_end();
  }
}

/*
  do_uart_irq handles a UART IRQ exception.
*/
void do_uart_irq() {
  uint32_t mis = uart.regs->mis;

  if (mis & MIS_TXMIS) {
    do_uart_irq_transmit();
  }
}
