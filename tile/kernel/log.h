#ifndef LOG_H
#define LOG_H

void log_put_signed_integer(long long a);
void log_put_unsigned_integer(unsigned long long a, const char format);

int log_putstring(const char* s);
int log_puts(const char* s);

int log_printf(const char *format, ...);

#endif
