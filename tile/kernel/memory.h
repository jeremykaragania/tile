#ifndef MEMORY_H
#define MEMORY_H

#include <kernel/asm/memory.h>
#include <kernel/asm/page.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_MAP_GROUP_LENGTH 128

#define ALIGN(a, b) ((a + b - 1) & ~(b - 1))
#define IS_ALIGNED(a, b) (ALIGN(a, b) == a)
#define bitmap_index(addr) (addr / PAGE_SIZE / 32)
#define bitmap_index_index(addr) (addr / PAGE_SIZE % 32)
#define bitmap_to_addr(i, j) (i * PAGE_SIZE * 32 + PAGE_SIZE * j)

const extern uint32_t* PHYS_OFFSET;
const extern uint32_t* VIRT_OFFSET;

const extern uint32_t* text_begin;
const extern uint32_t* text_end;
const extern uint32_t* data_begin;
const extern uint32_t* data_end;
const extern uint32_t* vector_table_begin;
const extern uint32_t* vector_table_end;
const extern uint32_t* interrupts_begin;
const extern uint32_t* interrupts_end;
const extern uint32_t* bss_begin;
const extern uint32_t* bss_end;

extern struct memory_map memory_map;
extern struct memory_manager memory_manager;

extern uint32_t high_memory;

/*
  enum memory_flags represents the attributes of a memory region.
*/
enum memory_flags {
  BLOCK_RWX,
  BLOCK_RW,
  BLOCK_RO
};

/*
  enum memory_map_block_flags represents the attribute of a memory block.
*/
enum memory_map_block_flags {
  BLOCK_NONE = 0x0,
  BLOCK_RESERVED = 0x1,
};

/*
  struct memory_map_block represents the bounds of a memory block.
*/
struct memory_map_block {
  uint64_t begin;
  uint64_t size;
  int flags;
};

/*
  struct memory_map_group represents a group of memory blocks.
*/
struct memory_map_group {
  size_t size;
  struct memory_map_block* blocks;
};

/*
  struct memory_map represents the memory map of a machine.
*/
struct memory_map {
  uint64_t limit;
  struct memory_map_group* memory;
  struct memory_map_group* reserved;
};

/*
  struct memory_manager represents the virtual memory information of a process.
*/
struct memory_manager {
  uint32_t* pgd;
  uint32_t text_begin;
  uint32_t text_end;
  uint32_t data_begin;
  uint32_t data_end;
  uint32_t bss_begin;
  uint32_t bss_end;
};

uint64_t virt_to_phys(uint32_t x);
uint32_t phys_to_virt(uint64_t x);

void init_memory_map();
void init_memory_manager(void* pgd, void* text_begin, void* text_end, void* data_begin, void* data_end, void* bss_begin, void* bss_end);

void update_memory_map();

void memory_map_mask_block(struct memory_map_block* block, int flag, int mask);
void memory_map_merge_blocks(struct memory_map_group* group, int begin, int end);
void memory_map_insert_block(struct memory_map_group* group, int pos, uint64_t begin, uint64_t size);
void memory_map_add_block(struct memory_map_group* group, uint64_t begin, uint64_t size);
int memory_map_split_block(struct memory_map_group* group, uint64_t begin);

void bitmap_insert(uint32_t* bitmap, uint32_t addr, uint32_t size);
void bitmap_clear(uint32_t* bitmap, uint32_t addr);

void* memory_phys_alloc(size_t size);
void* memory_alloc(size_t size);
int memory_free(void* ptr);

#endif
