#include <uart.h>

volatile struct uart_registers* uart_0 = (volatile struct uart_registers*)0xffc90000;

char uart_buf[256];

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

int uart_putchar(const int c) {
  while(uart_0->fr & FR_TXFF);
  uart_0->dr = c;
  return c;
}

void uart_putint(int a) {
  const unsigned int ascii_offset = 48;
  if (a == 0) {
    uart_putchar('0');
  }
  else {
    size_t i = 0;
    size_t j;
    if (a < 0) {
      uart_putchar('-');
      a *= -1;
    }
    while (a) {
      char d = (a % 10) + ascii_offset;
      uart_buf[i] = d;
      a /= 10;
      ++i;
    }
    for (j = 0; j < i; ++j) {
      uart_putchar(uart_buf[i-j-1]);
    }
  }
}

void uart_putuint(unsigned int a) {
  if (a == 0) {
    uart_putchar('0');
  }
  else {
    size_t i = 0;
    size_t j;
    const unsigned int ascii_offset = 48;
    while (a) {
      char d = (a % 10) + ascii_offset;
      uart_buf[i] = d;
      a /= 10;
      ++i;
    }
    for (j = 0; j < i; ++j) {
      uart_putchar(uart_buf[i-j-1]);
    }
  }
}

void uart_puthex(unsigned int a, const char format) {
  size_t i = 0;
  size_t j;
  if (a == 0) {
    ++i;
    uart_buf[0] = '0';
  }
  else {
    while (a) {
      unsigned int ascii_offset = 48;
      char d = a % 16;
      if (d > 9) {
        if (format == 'x') {
          ascii_offset = 87;
        }
        else {
          ascii_offset = 55;
        }
      }
      d += ascii_offset;
      uart_buf[i] = d;
      a /= 16;
      ++i;
    }
  }
  uart_putchar('0');
  uart_putchar(format);
  for (j = 0; j < i; ++j) {
    uart_putchar(uart_buf[i-j-1]);
  }
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

int uart_printf(const char *format, ...) {
  va_list args;
  size_t i = 0;
  va_start(args, format);
  while (format[i]) {
    char c = format[i];
    if (c == '%') {
      ++i;
      switch (format[i]) {
        case 'd': {
          uart_putint(va_arg(args, int));
          break;
        }
        case 'i': {
          uart_putint(va_arg(args, int));
          break;
        }
        case 'x': {
          uart_puthex(va_arg(args, unsigned int), 'x');
          break;
        }
        case 'X': {
          uart_puthex(va_arg(args, unsigned int), 'X');
          break;
        }
        case 'u': {
          uart_putuint(va_arg(args, unsigned int));
          break;
        }
        default: {
          uart_putchar('%');
          uart_putchar(format[i]);
        }
      }
    }
    else {
      uart_putchar(c);
    }
    ++i;
  }
  return i;
}

int uart_getchar() {
  if (uart_0->fr & FR_RXFE) {
    return -1;
  }
  return uart_0->dr & DR_DATA;
}
