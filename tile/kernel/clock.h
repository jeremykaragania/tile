#ifndef CLOCK_H
#define CLOCK_H

#include <kernel/asm/clock.h>
#include <stdint.h>

/*
  clock_info represents clock information.
*/
struct clock_info {
  uint32_t frequency_mhz;
};

extern void init_clock();
extern uint64_t get_counter();
extern uint32_t get_timer();

extern struct clock_info clock_info;

uint64_t get_ns();

#endif
