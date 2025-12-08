#include <kernel/log.h>
#include <kernel/processor.h>

/*
  panic handles kernel panics. Currently it just enters an endless loop.
*/
void panic(const char* s) {
  log_printf("%s", s);
  while(1);
}
