#ifndef CLOCK_H
#define CLOCK_H

#include <kernel/asm/clock.h>
#include <stdint.h>

extern uint64_t get_counter();
extern struct clock_info clock_info;

/*
  clock_info represents clock information.
*/
struct clock_info {
  uint32_t frequency;
};

#endif
