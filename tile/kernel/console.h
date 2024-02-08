#ifndef CONSOLE_H
#define CONSOLE_H

#define VE_UART0 0x1c090000

#include <stddef.h>
#include <stdint.h>

int console_putchar(int c);

int console_puts(char* s);

#endif
