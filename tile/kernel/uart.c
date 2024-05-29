#include <uart.h>

volatile struct uart_registers* uart_0 = (volatile struct uart_registers*)0xffc00000;

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

void uart_put_signed_integer(long long a) {
  if (a < 0) {
    uart_putchar('-');
    a *= -1;
  }
  uart_put_unsigned_integer(a, 'd');
}

void uart_put_unsigned_integer(unsigned long long a, const char format) {
  if (a == 0) {
    uart_putchar('0');
  }
  else {
    unsigned int base;
    unsigned int offsets[2] = {48, 0};
    size_t i = 0;

    switch (format) {
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

    if (format == 'x' || format == 'X') {
      uart_putchar('0');
      uart_putchar(format);
    }
    for (size_t j = 0; j < i; ++j) {
      uart_putchar(uart_buf[i-j-1]);
    }
  }
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
        case 'x': {
          switch (length) {
            case LM_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long), 'x');
              break;
            }
            case LM_LONG_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long long), 'x');
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              uart_put_unsigned_integer(va_arg(args, unsigned int), 'x');
              break;
            }
          }
          break;
        }
        case 'X': {
          switch (length) {
            case LM_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long), 'X');
              break;
            }
            case LM_LONG_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long long), 'X');
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              uart_put_unsigned_integer(va_arg(args, unsigned int), 'X');
              break;
            }
          }
          break;
        }
        case 'u': {
          switch (length) {
            case LM_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long), 'd');
              break;
            }
            case LM_LONG_LONG: {
              uart_put_unsigned_integer(va_arg(args, unsigned long long), 'd');
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              uart_put_unsigned_integer(va_arg(args, unsigned int), 'd');
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

int uart_getchar() {
  if (uart_0->fr & FR_RXFE) {
    return -1;
  }
  return uart_0->dr & DR_DATA;
}
