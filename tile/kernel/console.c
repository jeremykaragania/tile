#include <console.h>

volatile uint32_t* ve_uart0 = (volatile uint32_t*)0x1c090000;

int console_putchar(const int c) {
  *ve_uart0 = c;
  return c;
}

int console_puts(const char* s) {
  size_t i = 0;
  while (s[i]) {
    console_putchar(s[i]);
    ++i;
  }
  console_putchar('\n');
  return 0;
}
