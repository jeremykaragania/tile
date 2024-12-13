#include <kernel/clock.h>

struct clock_info clock_info = {
  SYS_CLK_MHZ
};

/*
  get_ns returns the elapsed nanoseconds.
*/
uint64_t get_ns() {
  return get_counter() / clock_info.frequency_mhz * 1000;
}
