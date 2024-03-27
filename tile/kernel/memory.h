#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

const extern uint32_t* PHYS_OFFSET;

const extern uint32_t* VIRT_OFFSET;

struct memory_block {
  uint32_t begin;
  uint32_t size;
};

struct memory_region {
  unsigned long length;
  struct memory_block* blocks;
};

struct memory_info {
  struct memory_region* memory;
  struct memory_region* reserved;
};

extern struct memory_info memory_info;

extern uint32_t virt_to_phys(uint32_t x);

extern uint32_t phys_to_virt(uint32_t x);

#endif
