#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

const extern uint32_t* PHYS_OFFSET;

const extern uint32_t* VIRT_OFFSET;

struct memory_info {
  const uint32_t text_begin;
  const uint32_t text_end;
  const uint32_t data_begin;
  const uint32_t data_end;
};

extern uint32_t virt_to_phys(uint32_t x);

extern uint32_t phys_to_virt(uint32_t x);

#endif
