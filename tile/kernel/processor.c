#include <drivers/pl011.h>
#include <kernel/processor.h>

/*
  panic handles kernel panics. Currently it just enters an endless loop.
*/
void panic(const char* s) {
  uart_printf(s);
  while(1);
}
