#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define MEMORY_MAP_GROUP_LENGTH 128

const extern uint32_t* KERNEL_SPACE_PADDR;
const extern uint32_t* PG_DIR_PADDR;
const extern uint32_t* PG_DIR_SIZE;

const extern uint32_t* text_begin;
const extern uint32_t* text_end;
const extern uint32_t* data_begin;
const extern uint32_t* data_end;
const extern uint32_t* bss_begin;
const extern uint32_t* bss_end;

extern uint32_t virt_to_phys(uint32_t x);
extern uint32_t phys_to_virt(uint32_t x);

extern struct memory_map memory_map;
extern struct memory_manager init_memory_manager;

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
  size_t length;
  struct memory_map_block* blocks;
};

/*
  struct memory_map represents the memory map of a machine.
*/
struct memory_map {
  struct memory_map_group* memory;
  struct memory_map_group* reserved;
};

/*
  struct memory_manager represents the virtual memory information of a process.
*/
struct memory_manager {
  uint32_t* pg_dir;
  uint32_t text_begin;
  uint32_t text_end;
  uint32_t data_begin;
  uint32_t data_end;
};

void init_memory_map();
void init_init_memory_manager(void* pg_dir, void* text_begin, void* text_end, void* data_begin, void* data_end);
void memory_map_merge_blocks(struct memory_map_group* group, int begin, int end);
void memory_map_insert_block(struct memory_map_group* group, int pos, uint32_t begin, uint32_t size);
void memory_map_add_block(struct memory_map_group* group, uint32_t begin, uint32_t size);

#endif
