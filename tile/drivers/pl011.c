/*
  pl011.c provides an ARM PrimeCell UART (PL011) driver.
*/

#include <drivers/pl011.h>

/*
  The UART is mapped to this initial address as this is where it is mapped in
  the initial memory map before the proper memory mapping is initialized.
*/
volatile struct uart_registers* uart_0 = (volatile struct uart_registers*)0xffc90000;

char uart_buf[256];

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
  uart_put_signed_integer converts a signed integer "a" into a string and
  writes it to the UART data register.
*/
void uart_put_signed_integer(long long a) {
  if (a < 0) {
    uart_putchar('-');
    a *= -1;
  }
  uart_put_unsigned_integer(a, 'd');
}

/*
  uart_put_unsigned_integer converts an unsigned integer "a" into a string
  depending on a format "format" and writes it to the UART data register.
*/
void uart_put_unsigned_integer(unsigned long long a, const char format) {
  if (a == 0) {
    uart_putchar('0');
  }
  else {
    unsigned int base;
    unsigned int offsets[2] = {48, 0};
    size_t i = 0;

    switch (format) {
      case 'o': {
        base = 8;
        break;
      }
      case 'x': {
        offsets[1] = 87;
        base = 16;
        break;
      }
      case 'X': {
        offsets[1] = 55;
        base = 16;
        break;
      }
      default : {
        base = 10;
        break;
      }
    }

    while (a) {
      char d = (a % base);
      if (d > 9) {
        d += offsets[1];
      }
      else {
        d += offsets[0];
      }
      uart_buf[i] = d;
      a /= base;
      ++i;
    }

    for (size_t j = 0; j < i; ++j) {
      uart_putchar(uart_buf[i-j-1]);
    }
  }
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
  uart_putstring writes a string "s" to the UART data register.
*/
int uart_putstring(const char* s) {
  size_t i = 0;
  while (s[i]) {
    uart_putchar(s[i]);
    ++i;
  }
  return i;
}

/*
  uart_puts writes a string "s" followed by a newline character to the UART
  data register.
*/
int uart_puts(const char* s) {
  uart_putstring(s);
  uart_putchar('\n');
  return 0;
}

/*
  uart_printf writes formatted data "format" to the UART data register.
*/
int uart_printf(const char *format, ...) {
  va_list args;
  size_t i = 0;
  int length = LM_NONE;

  va_start(args, format);

  while (format[i]) {
    char c = format[i];

    if (c == '%') {
      ++i;

      switch (format[i]) {
        case 'h': {
         length = LM_SHORT;
          ++i;

          if (format[i] == 'h') {
            length = LM_CHAR;
            ++i;
          }

          break;
        }
        case 'l': {
         length = LM_LONG;
         ++i;

          if (format[i] == 'l') {
            length = LM_LONG_LONG;
            ++i;
          }

          break;
        }
      }

      switch (format[i]) {
        case 'c': {
          uart_putchar(va_arg(args, int));
          break;
        }
        case 's': {
          uart_putstring(va_arg(args, char*));
          break;
        }
        case 'd':
        case 'i': {
          switch (length) {
            case LM_LONG: {
              uart_put_signed_integer(va_arg(args, long));
              break;
            }
            case LM_LONG_LONG: {
              uart_put_signed_integer(va_arg(args, long long));
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              uart_put_signed_integer(va_arg(args, int));
              break;
            }
          }
          break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
          switch (length) {
            case LM_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long), format[i]);
              break;
            }
            case LM_LONG_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long long), format[i]);
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              uart_put_unsigned_integer(va_arg(args, unsigned int), format[i]);
              break;
            }
          }
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
