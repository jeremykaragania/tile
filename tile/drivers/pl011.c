/*
  pl011.c provides an ARM PrimeCell UART (PL011) driver.
*/

#include <drivers/pl011.h>
#include <kernel/device.h>
#include <kernel/memory.h>
#include <lib/string.h>

struct uart_operations uart_operations = {
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
};

/*
  The UART device is never exposed. Instead, it's accessed through its terminal
  device.
*/
struct device uart_device = {
  .name = "console",
  .major = 5,
  .minor = 1,
  .type = DT_CHARACTER,
  .private = &uart
};

/*
  uart_init initializes the UART.
*/
void uart_init() {
  struct terminal* term;
  struct device* term_dev;

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

  /* Initialize the terminal structure. */
  term = terminal_alloc();
  term->ops = uart.ops;
  term->private = &uart;
  uart.term = term;

  /* Initialize the terminal device. */
  term_dev = memory_alloc(sizeof(struct device));
  strcpy(term_dev->name, uart_device.name);
  term_dev->ops = &terminal_operations;
  term_dev->major = uart_device.major;
  term_dev->minor = uart_device.minor;
  term_dev->type = uart_device.type;
  term_dev->private = term;

  /* The terminal, not the UART is exposed. */
  device_register(term_dev);
}

/*
  uart_read reads up to "count" bytes into the buffer "buf" from the UART.
*/
int uart_read(struct uart* u, void* buf, size_t count) {
  int c;

  for (size_t i = 0; i < count; ++i) {
    c = -1;

    while (c < 0) {
      c = uart_getchar(u);
    }

    ((char*)buf)[i] = c;
  }

  return count;
}

/*
  uart_write writes up to "count" bytes from the buffer "buf" to the UART.
*/
int uart_write(struct uart* u, const void* buf, size_t count) {
  int ret;

  ret = fifo_push_n(&u->fifo, buf, count);

  uart_begin(u);
  do_uart_irq_transmit(u);

  return ret;
}

/*
  uart_putchar writes a byte "c" to the UART data register.
*/
int uart_putchar(struct uart* u, const int c) {
  while(u->regs->fr & FR_TXFF);
  u->regs->dr = c;
  return c;
}

/*
  uart_getchar reads a byte from the UART data register. It returns the read
  byte on success or -1 on failure.
*/
int uart_getchar(struct uart* u) {
  if (u->regs->fr & FR_RXFE) {
    return -1;
  }
  return u->regs->dr & DR_DATA;
}

/*
  uart_begin begins UART transmission.
*/
void uart_begin(struct uart* u) {
  u->regs->imsc = IMSC_TXIM;
}

/*
  uart_end ends UART transmission.
*/
void uart_end(struct uart* u) {
  u->regs->imsc = ~IMSC_TXIM;
}

/*
  do_uart_irq_transmit handles a UART transmit IRQ exception.
*/
void do_uart_irq_transmit(struct uart* u) {
  char c;

  while (1) {
    if (u->regs->fr & FR_TXFF) {
      break;
    }

    if (fifo_pop(&u->fifo, &c) < 0) {
      break;
    }

    u->regs->dr = c;
  }

  if (is_fifo_empty(&u->fifo)) {
    uart_end(u);
  }
}

/*
  do_uart_irq_receive handles a UART receive IRQ exception.
*/
void do_uart_irq_receive(struct uart* u) {
  char c;

  u->regs->icr |= ICR_RXIC;

  while (!(u->regs->fr & FR_RXFE)) {
    c = u->regs->dr & DR_DATA;
    fifo_push(&u->term->fifo, &c);
  }
}

/*
  do_uart_irq handles a UART IRQ exception.
*/
void do_uart_irq(struct uart* u) {
  uint32_t mis = u->regs->mis;

  if (mis & MIS_TXMIS) {
    do_uart_irq_transmit(u);
  }

  if (mis & MIS_RXMIS) {
    do_uart_irq_receive(u);
  }
}
