#include <drivers/pl011.h>
#include <kernel/log.h>

char log_buf[256];

/*
  log_put_signed_integer converts a signed integer "a" into a string and logs
  it.
*/
void log_put_signed_integer(long long a) {
  if (a < 0) {
    uart_putchar('-');
    a *= -1;
  }
  log_put_unsigned_integer(a, 'd');
}

/*
  log_put_unsigned_integer converts an unsigned integer "a" into a string
  depending on a format "format" and logs it.
*/
void log_put_unsigned_integer(unsigned long long a, const char format) {
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
      log_buf[i] = d;
      a /= base;
      ++i;
    }

    for (size_t j = 0; j < i; ++j) {
      uart_putchar(log_buf[i-j-1]);
    }
  }
}

/*
  log_putstring logs a string "s".
*/
int log_putstring(const char* s) {
  size_t i = 0;
  while (s[i]) {
    uart_putchar(s[i]);
    ++i;
  }
  return i;
}

/*
  log_puts logs a string "s" followed by a newline character.
*/
int log_puts(const char* s) {
  log_putstring(s);
  uart_putchar('\n');
  return 0;
}

/*
  log_printf logs formatted data "format".
*/
int log_printf(const char *format, ...) {
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
          log_putstring(va_arg(args, char*));
          break;
        }
        case 'd':
        case 'i': {
          switch (length) {
            case LM_LONG: {
              log_put_signed_integer(va_arg(args, long));
              break;
            }
            case LM_LONG_LONG: {
              log_put_signed_integer(va_arg(args, long long));
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              log_put_signed_integer(va_arg(args, int));
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
              log_put_unsigned_integer(va_arg(args, unsigned long), format[i]);
              break;
            }
            case LM_LONG_LONG: {
              log_put_unsigned_integer(va_arg(args, unsigned long long), format[i]);
              break;
            }
            case LM_CHAR:
            case LM_SHORT:
            default: {
              log_put_unsigned_integer(va_arg(args, unsigned int), format[i]);
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
