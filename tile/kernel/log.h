#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

/*
  enum length_modifier represents a format string's length modifiers.
*/
enum length_modifier {
  LM_NONE,
  LM_CHAR,
  LM_SHORT,
  LM_LONG,
  LM_LONG_LONG
};

void log_put_signed_integer(long long a);
void log_put_unsigned_integer(unsigned long long a, const char format);

int log_putstring(const char* s);
int log_puts(const char* s);
int log_putchar(const char c);

int log_printf(const char *format, ...);

#endif
