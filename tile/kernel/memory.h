#ifndef MEMORY_H
#define MEMORY_H

#include <kernel/asm/memory.h>
#include <kernel/asm/page.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_MAP_GROUP_LENGTH 128

/*
  virt_to_phys returns a physical address from a virtual address "x".
*/
#define virt_to_phys(x) ((x) - (VIRT_OFFSET - PHYS_OFFSET))

/*
  phys_to_virt returns a virtual address from a physical address "x".
*/
#define phys_to_virt(x) ((x) + (VIRT_OFFSET - PHYS_OFFSET))

#define page_count(x) ((x - 1) / PAGE_SIZE + 1)
#define page_index(x) ((x) >> PAGE_SHIFT)
#define page_addr(x) ((x) << PAGE_SHIFT)

#define ALIGN(a, b) ((a + b - 1) & ~(b - 1))
#define IS_ALIGNED(a, b) (ALIGN(a, b) == a)

/*
  enum memory_flags represents the attributes of a memory region.
*/
enum memory_flags {
  BLOCK_RWX,
  BLOCK_RW,
  BLOCK_RO
};

/*
  enum initmem_block_flags represents the attribute of a memory block.
*/
enum initmem_block_flags {
  BLOCK_NONE = 0x0,
  BLOCK_RESERVED = 0x1,
};

/*
  struct initmem_block represents the bounds of a memory block.
*/
struct initmem_block {
  uint32_t begin;
  uint32_t size;
  int flags;
  struct initmem_block* next;
  struct initmem_block* prev;
};

/*
  struct initmem_group represents a group of memory blocks.
*/
struct initmem_group {
  size_t size;
  struct initmem_block* blocks;
};

/*
  struct memory_map represents the memory map of a machine.
*/
struct initmem_info {
  uint32_t limit;
  struct initmem_group* memory;
  struct initmem_group* reserved;
};

enum memory_page_flags {
  PAGE_NONE = 0x0,
  PAGE_RESERVED = 0x1
};

/*
  struct memory_page_info represents the information of a page use for memory
  allocation.
*/
struct memory_page_info {
  int flags;
  void* data;
  struct memory_page_info* next;
};

/*
  struct page_group represents a group of pages. Page groups are linked into a
  linked list with successive elements mapping higher addresses. Each page
  group contains the underlying page information data "pages"; its size "size";
  the first address "offset"; and a pointer to the next page group "next".
*/
struct page_group {
  struct memory_page_info* pages;
  uint32_t size;
  uint32_t offset;
  struct page_group* next;
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

extern struct initmem_info initmem_info;
extern struct memory_manager memory_manager;
extern struct memory_page_info memory_page_infos;

extern struct page_group* page_groups;
extern struct memory_page_info* memory_pages;

extern uint32_t high_memory;

void initmem_init();
void memory_manager_init(void* pgd, void* text_begin, void* text_end, void* data_begin, void* data_end, void* bss_begin, void* bss_end);
void memory_alloc_init();

void update_memory_map();

void initmem_mask_block(struct initmem_block* block, int flag, int mask);
void initmem_merge_blocks(struct initmem_group* group, int begin, int end);
void initmem_insert_block(struct initmem_group* group, int pos, uint32_t begin, uint32_t size);
void initmem_add_block(struct initmem_group* group, uint32_t begin, uint32_t size);
int initmem_split_block(struct initmem_group* group, uint32_t begin);

size_t page_group_index(const struct page_group* group, uint32_t addr);
uint32_t page_group_addr(const struct page_group* group, uint32_t index);
uint32_t page_group_end(const struct page_group* group);
struct memory_page_info* page_group_get(const struct page_group* group, uint32_t addr);
int page_group_is_free(const struct page_group* group, uint32_t addr, size_t count);
void page_group_insert(struct page_group* group, uint32_t addr, size_t count);
void page_group_clear(struct page_group* group, uint32_t addr, size_t count);
void* page_group_alloc(struct page_group* group, uint32_t begin, size_t count, size_t align, size_t gap);

void* initmem_phys_alloc(size_t size);
void* initmem_alloc(size_t size);
int initmem_free(void* ptr);

void* memory_page_alloc(size_t count);
void* memory_block_alloc(size_t size);

void* memory_page_data_alloc();
void* memory_block_page_alloc(struct memory_page_info* page, size_t size, size_t align);

void* memory_alloc(size_t size);
int memory_free(void* ptr);

#endif
