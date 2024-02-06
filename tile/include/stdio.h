#ifndef STDIO_H
#define STDIO_H

#include <stdint.h>

int putchar(int c) {
  *(uint32_t *)(0x1c090000) = c;
  return c;
}

#endif
