#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

const extern uint32_t* PHYS_OFFSET;

const extern uint32_t* VIRT_OFFSET;

const extern uint32_t* KERNEL_SPACE_PADDR;

extern struct memory_map_info memory_map_info;

extern uint32_t virt_to_phys(uint32_t x);

extern uint32_t phys_to_virt(uint32_t x);

/*
  struct memory_map_block represents the bounds of a memory region.
*/
struct memory_map_block {
  uint32_t begin;
  uint32_t size;
};

/*
  struct memory_map_group represents a group of memory regions.
*/
struct memory_map_group {
  unsigned long length;
  struct memory_map_block* blocks;
};

/*
  struct memory_map_info represents the memory map of a machine.
*/
struct memory_map_info {
  struct memory_map_group* memory;
  struct memory_map_group* reserved;
};

#endif
