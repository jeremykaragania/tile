#include <console.h>

int console_putchar(int c) {
  *(uint32_t*)(VE_UART0) = c;
  return c;
}

int console_puts(char* s) {
  size_t i = 0;
  while (s[i]) {
    console_putchar(s[i]);
    ++i;
  }
  console_putchar('\n');
  return 0;
}
