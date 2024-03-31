#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>

const extern uint32_t* PHYS_OFFSET;
const extern uint32_t* VIRT_OFFSET;

const extern uint32_t* KERNEL_SPACE_PADDR;
const extern uint32_t* SMC_VADDR;
const extern uint32_t* KERNEL_SPACE_VADDR;
const extern uint32_t* PG_DIR_VADDR;

const extern uint32_t* text_begin;
const extern uint32_t* text_end;
const extern uint32_t* data_begin;
const extern uint32_t* data_end;

extern uint32_t virt_to_phys(uint32_t x);
extern uint32_t phys_to_virt(uint32_t x);

extern struct memory_map_info memory_map_info;
extern struct memory_manager_info init_memory_manager_info;

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

/*
  struct memory_manager_info represents the memory information of a process.
*/
struct memory_manager_info {
  uint32_t* pg_dir;
  uint32_t text_begin;
  uint32_t text_end;
  uint32_t data_begin;
  uint32_t data_end;
};

#endif
