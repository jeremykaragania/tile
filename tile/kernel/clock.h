#ifndef CLOCK_H
#define CLOCK_H

#include <kernel/asm/clock.h>
#include <stdint.h>

extern void init_clock();
extern uint64_t get_counter();

extern struct clock_info clock_info;

/*
  clock_info represents clock information.
*/
struct clock_info {
  uint32_t frequency_mhz;
};

uint64_t get_ns();

#endif
