#include <kernel/processor.h>

/*
  panic handles kernel panics. Currently it just enters an endless loop.
*/
void panic() {
  while(1);
}
