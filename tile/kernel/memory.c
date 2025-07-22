/*
  memory.c handles physical memory management.

  There are two memory allocators: a secondary memory allocator and a primary
  memory allocator. The secondary memory allocator bootstraps the primary
  memory allocator.

  The secondary memory allocator initializes the memory map of the machine. It
  deals with memory groups which are abstractions of memory. It's heavily
  inspired by Linux's boot time memory management and memblock allocator.

  The primary memory allocator is a first-fit allocator. It uses pages for
  backing memory. These pages serve as the foundation for further, more
  granular allocations to be made on top of.

  Both memory allocators deal with memory blocks which represent contiguous
  free or reserved memory. A memory block is defined by its beginning and its
  size in bytes. Memory blocks are stored in a doubly linked which is sorted by
  the bounds of its entries.
*/

#include <kernel/memory.h>
#include <lib/string.h>

static struct initmem_block initmem_memory_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct initmem_group initmem_memory_group;

static struct initmem_block initmem_reserved_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct initmem_group initmem_reserved_group;

struct initmem_info initmem_info;

struct memory_manager memory_manager;

struct page_group* page_groups;

struct memory_page_info memory_page_infos;

uint32_t high_memory;

/*
  initmem_init initializes the initial memory allocator.
*/
void initmem_init() {
  /*
    We initialize the kernel's initial memory allocator using the system's
    memory map; and the kernel's memory map from the linker. Eventually we
    won't need this memory allocator, we just need it now to allocate the page
    groups.
  */
  initmem_memory_group.size = 0;
  initmem_memory_group.blocks = initmem_memory_blocks;

  initmem_reserved_group.size = 0;
  initmem_reserved_group.blocks = initmem_reserved_blocks;

  initmem_info.limit = 0xffffffff;
  initmem_info.memory = &initmem_memory_group;
  initmem_info.reserved = &initmem_reserved_group;

  initmem_add_block(initmem_info.memory, KERNEL_SPACE_PADDR, 0x80000000);
  initmem_add_block(initmem_info.reserved, 0x80000000, 0x8000);
  initmem_add_block(initmem_info.reserved, PG_DIR_PADDR, PG_DIR_SIZE);
  initmem_add_block(initmem_info.reserved, virt_to_phys((uint32_t)&text_begin), (uint32_t)&bss_end - (uint32_t)&text_begin);
}

/*
  memory_manager_init initializes the kernel's memory manager.
*/
void memory_manager_init(void* pgd, void* text_begin, void* text_end, void* data_begin, void* data_end, void* bss_begin, void* bss_end) {
  memory_manager.pgd = (uint32_t*)(uint32_t)pgd;
  memory_manager.text_begin = (uint32_t)text_begin;
  memory_manager.text_end = (uint32_t)text_end;
  memory_manager.data_begin = (uint32_t)data_begin;
  memory_manager.data_end = (uint32_t)data_end;
  memory_manager.bss_begin = (uint32_t)bss_begin;
  memory_manager.bss_end = (uint32_t)bss_end;
}


/*
  update_memory_map iterates over the memory map and invalidates blocks for
  memory allocation. It then finds the new bounds of low memory and high
  memory, which should be interpreted under the Linux meaning.
*/
void update_memory_map() {
  struct initmem_block* b;
  uint32_t b_end;
  uint32_t vmalloc_min_paddr = virt_to_phys(VMALLOC_MIN_VADDR);
  uint32_t lowmem_end = 0;

  for (size_t i = 0; i < initmem_info.memory->size; ++i) {
    b = &initmem_info.memory->blocks[i];

    if (!IS_ALIGNED(b->begin, PAGE_SIZE)) {
      initmem_mask_block(b, BLOCK_RESERVED, 1);
    }
  }

  for (size_t i = 0; i < initmem_info.memory->size; ++i) {
    b_end = b->begin + b->size - 1;

    if (!(b->flags & BLOCK_RESERVED)) {
      if (b->begin < vmalloc_min_paddr) {
        if (b_end > vmalloc_min_paddr) {
          lowmem_end = vmalloc_min_paddr;
        }
        else {
          lowmem_end = b_end;
        }
      }
    }
  }

  high_memory = phys_to_virt(lowmem_end);
  initmem_info.limit = lowmem_end;
  initmem_split_block(initmem_info.memory, initmem_info.limit);
  lowmem_end -= 1;
}

/*
  mem_init initializes the primary memory allocator.
*/
void memory_alloc_init() {
  struct initmem_block* block;
  struct memory_page_info* page;
  struct page_group* curr;
  struct page_group* prev;

  curr = NULL;
  prev = NULL;

  /*
    For each memory block in the initial memory map's memory group, we allocate
    a physical page page_group to map that memory block.
  */
  for (size_t i = 0; i < initmem_info.memory->size; ++i) {
    block = &initmem_info.memory->blocks[i];
    curr = initmem_alloc(sizeof(struct page_group));

    if (!prev) {
      page_groups = curr;
    }
    else {
      prev->next = curr;
    }

    curr->pages = initmem_alloc((block->size >> PAGE_SHIFT) * sizeof(struct memory_page_info));
    curr->size = block->size;
    curr->offset = block->begin;
    curr->next = NULL;

    prev = curr;
  }

  curr = page_groups;

  /*
    For each memory block in the initial memory map's reserved group, we
    reserve that memory block in the appropriate physical page page_group.
  */
  for (size_t i = 0; i < initmem_info.reserved->size; ++i) {
    block = &initmem_info.reserved->blocks[i];

    while (block->begin > page_group_end(curr)) {
      curr = curr->next;
    }

    for (size_t j = page_group_index(curr, block->begin); j < page_group_index(curr, ALIGN(block->begin + block->size, PAGE_SIZE)); ++j) {
      page = &curr->pages[j];
      page->flags = PAGE_RESERVED;
    }
  }

  memory_page_infos.next = NULL;
  memory_page_infos.data = memory_page_data_alloc();
}

/*
  initmem_mask_block sets a flag "flag" to "mask" in "block".
*/
void initmem_mask_block(struct initmem_block* block, int flag, int mask) {
  if (mask) {
    block->flags |= flag;
  }
  else {
    block->flags &= ~flag;
  }
}

/*
  initmem_merge_blocks merges the blocks in "group" from "begin" to "end".
*/
void initmem_merge_blocks(struct initmem_group* group, int begin, int end) {
  size_t merged = 0;
  struct initmem_block* a = &group->blocks[begin];
  struct initmem_block* b;
  size_t a_end;
  size_t b_end;

  /*
    We have two blocks to consider: "a": the block we try to merge into; and
    "b": the block after "a".
  */
  for (size_t i = begin+1; i < end; ++i) {
    b = &group->blocks[i];
    a_end = a->begin + a->size - 1;
    b_end = b->begin + b->size - 1;

    if (a_end > b->begin) {
      if (a_end < b_end) {
        a->size = b_end - a->begin;
      }

      group->blocks[i] = group->blocks[i+1];
      ++merged;
    }
    else {
      a = b;
    }

  }

  group->size -= merged;
}

/*
  initmem_insert_block inserts a block into "group" at "pos". The block is
  specified by "begin" and "size".
*/
void initmem_insert_block(struct initmem_group* group, int pos, uint32_t begin, uint32_t size) {
  struct initmem_block* block = &group->blocks[pos];

  memmove(block + 1, block, (group->size - pos) * sizeof(*block));
  block->begin = begin;
  block->size = size;
  ++group->size;
}

/*
  initmem_add_block adds a block into "group". The block is specified by
  "begin" and "size". The block is added such that the blocks in "group" remain
  sorted by their beginning.
*/
void initmem_add_block(struct initmem_group* group, uint32_t begin, uint32_t size) {
  struct initmem_block *b;
  uint32_t b_end;
  size_t pos = group->size;

  if (group->size == 0) {
    initmem_insert_block(group, pos, begin, size);
  }
  else {
    for (size_t i = 0; i < group->size; ++i) {
      b = &group->blocks[i];
      b_end = b->begin + b->size - 1;

      if (begin <= b_end) {
        pos = i;

        if (begin >= b->begin) {
          ++pos;
        }

        break;
      }
    }

    initmem_insert_block(group, pos, begin, size);
  }

  initmem_merge_blocks(group, 0, group->size);
}

/*
  initmem_split tries to split a block in "group" at "begin" into two
  discrete blocks: a left block and a right block. It returns the index of the
  left block in the group if successful.
*/
int initmem_split_block(struct initmem_group* group, uint32_t begin) {
  struct initmem_block* a;
  uint32_t a_end;

  for (size_t i = 0; i < group->size; ++i) {
    a = &group->blocks[i];
    a_end = a->begin + a->size - 1;

    if (begin > a->begin && begin < a_end) {
      uint32_t new_end;

      a->size = begin - a->begin;
      new_end = a->begin + a->size - 1;
      initmem_add_block(group, new_end + 1, a_end - new_end);
      return i;
    }
  }

  return -1;
}

/*
  page_group_index returns the page group entry index for the address "addr" in
  the page group "group".
*/
size_t page_group_index(const struct page_group* group, uint32_t addr) {
  return page_index(addr - group->offset);
}

/*
  page_group_to_addr returns the page address from a page group entry index
  "index" in the page group "group".
*/
uint32_t page_group_addr(const struct page_group* group, uint32_t index) {
  return page_addr(page_index(group->offset) + index);
}

/*
  page_group_end_addr returns the upper address bound which the page group
  "group" contains.
*/
uint32_t page_group_end(const struct page_group* group) {
  return group->offset + group->size - 1;
}

/*
  page_group_get returns the page information from a page group "group" and a
  page address "addr".
*/
struct memory_page_info* page_group_get(const struct page_group* group, uint32_t addr) {
  return &group->pages[page_group_index(group, addr)];
}

/*
  page_group_addr_is_free returns true if "count" contiguous pages are free
  from the address "addr" in the page group "group".
*/
int page_group_is_free(const struct page_group* group, uint32_t addr, size_t count) {
  if (addr >= group->offset && addr + count * PAGE_SIZE <= page_group_end(group)) {
    int is_free = 1;

    for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
      is_free &= !(page_group_get(group, addr)->flags & PAGE_RESERVED);

      if (!is_free) {
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

/*
  page_group_insert inserts "count" contiguous pages from the address "addr" in the
  page group "page_group".
*/
void page_group_insert(struct page_group* group, uint32_t addr, size_t count) {
  for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
    page_group_get(group, addr)->flags |= PAGE_RESERVED;
  }
}

/*
  page_group_clear unreserves "count" contiguous pages from the address "addr"
  in the page group "page_group".
*/
void page_group_clear(struct page_group* group, uint32_t addr, size_t count) {
  for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
    page_group_get(group, addr)->flags |= ~PAGE_RESERVED;
  }
}

/*
  page_group_alloc allocates "count" contiguous pages aligned to "align" pages
  with "gap" free pages before it in the page group "group" above "begin" and
  returns a pointer to it.
*/
void* page_group_alloc(struct page_group* group, uint32_t begin, size_t count, size_t align, size_t gap) {
  uint32_t addr;
  uint32_t gap_size = gap << PAGE_SHIFT;

  while (group) {
    for (size_t i = page_group_index(group, begin + gap_size); i < group->size >> PAGE_SHIFT; ++i) {
      addr = page_group_addr(group, i);

      /*
        The page is aligned to "align" pages and it has "gap" free pages
        before it.
      */
      if (addr % (align << PAGE_SHIFT) == 0 && page_group_is_free(group, addr - gap_size, count)) {
        page_group_insert(group, addr, count);
        return (void*)addr;
      }
    }

    group = group->next;
  }

  return NULL;
}

/*
  initmem_phys_alloc allocates a block of "size" bytes and returns a pointer
  to its physical address. Memory is allocated adjacent to reserved memory.
*/
void* initmem_phys_alloc(size_t size) {
  uint32_t a_begin;
  uint32_t a_end;
  struct initmem_block* m;
  uint32_t m_end;
  struct initmem_block* r;
  uint32_t r_end;
  size_t rs = initmem_info.reserved->size;

  /*
    We allocate a block with the constraints: "a" is the block which we are
    trying to allocate; "m" is the block where we can allocate; and "r" is the
    block where we can't allocate.
  */
  for (size_t i = 0; i < initmem_info.memory->size; ++i) {
    m = &initmem_info.memory->blocks[i];
    m_end = m->begin + initmem_info.memory->blocks[i].size - 1;
    a_begin = m->begin;
    a_end = a_begin + size - 1;

    if (a_end > initmem_info.limit) {
      break;
    }

    for (size_t j = 0; j < initmem_info.reserved->size; ++j) {
      r = &initmem_info.reserved->blocks[j];
      r_end = r->begin + r->size - 1;

      if (a_end < r->begin) {
        initmem_add_block(initmem_info.reserved, a_begin, size);
        return (void*)a_begin;
      }

      a_begin = r_end + 1;
      a_end = a_begin + size - 1;
    }
  }

  /* We try to allocate after the ending reserved block. */
  a_begin = initmem_info.reserved->blocks[rs-1].begin + initmem_info.reserved->blocks[rs-1].size;
  a_end = a_begin + size - 1;

  if (a_end < m_end) {
    initmem_add_block(initmem_info.reserved, a_begin, size);
    return (void*)a_begin;
  }

  return NULL;
}

/*
  initmem_alloc allocates a block of "size" bytes and returns a pointer to
  its virtual address. Memory is allocated adjacent to reserved memory.
*/
void* initmem_alloc(size_t size) {
  return (uint32_t*)phys_to_virt((uint32_t)initmem_phys_alloc(size));
}

/*
  initmem_free frees the block which "ptr" points to.
*/
int initmem_free(void* ptr) {
  struct initmem_block* block;

  for (size_t i = 0; i < initmem_info.reserved->size; ++i) {
    block = &initmem_info.reserved->blocks[i];

    if (block->begin == virt_to_phys((uint32_t)ptr)) {
      memmove(block, block + 1, (initmem_info.reserved->size - i) * sizeof(*block));
      --initmem_info.reserved->size;
      return 1;
    }
  }

  return 0;
}

/*
  memory_page_alloc allocates "count" contiguous pages and returns a pointer to
  them.
*/
void* memory_page_alloc(size_t count) {
  void* data;
  struct memory_page_info* curr;
  struct memory_page_info* page;
  struct initmem_block* block;
  struct initmem_block* head;

  if (!count) {
    return NULL;
  }

  /*
    Make sure that the page has a free page before it to store page information
    inside.
  */
  data = (void*)phys_to_virt((uint32_t)page_group_alloc(page_groups, PHYS_OFFSET, count, count, 1));
  page_group_insert(page_groups, virt_to_phys((uint32_t)data) - PAGE_SIZE, 1);

  if (!data) {
    return NULL;
  }

  head = (void*)((uint32_t)data - PAGE_SIZE);
  block = (void*)((uint32_t)data - sizeof(struct initmem_block));

  /* Initialize the block information to describe the page allocation. */
  block->begin = (uint32_t)data;
  block->size = count * PAGE_SIZE;
  block->flags = BLOCK_RW;
  block->next = NULL;
  block->prev = head;

  /* Initialize the dummy head block information to link to the previous block. */
  head->begin = (uint32_t)data - PAGE_SIZE;
  head->size = 0;
  head->flags = BLOCK_RW;
  head->next = block;
  head->prev = NULL;

  page = memory_block_alloc(sizeof(struct memory_page_info));
  page->data = head;
  page->next = NULL;

  curr = &memory_page_infos;
  while (curr->next) {
    curr = curr->next;
  }

  curr->next = page;

  return data;
}

/*
  memory_block_alloc allocates a memory region under a page size and returns a
  pointer to it.
*/
void* memory_block_alloc(size_t size) {
  struct memory_page_info* curr = &memory_page_infos;
  struct memory_page_info tmp;
  void* ret = NULL;
  size_t align = 1;

  if (!size || size > PAGE_SIZE - sizeof(struct initmem_block)) {
    return NULL;
  }

  if (IS_ALIGNED(size, 2)) {
    align = size;
  }

  /*
    We try to allocate in one of the existing pages.
  */
  while (curr) {
    if ((ret = memory_block_page_alloc(curr, size, align))) {
      return ret;
    }

    /*
      If we've tried all of the pages, then we allocate a new one and try to
      allocate inside of it.
    */
    if (!curr->next) {
      break;
    }

    curr = curr->next;
  }

  /*
    To allocate the actual page information, we first need to allocate a
    page, allocate that page information within it, and then initialize that
    page information.
  */
  tmp.data = memory_page_data_alloc();
  tmp.next = NULL;
  curr->next = memory_block_page_alloc(&tmp, sizeof(struct memory_page_info), 1);
  curr->next->data = tmp.data;
  curr->next->next = NULL;

  if ((ret = memory_block_page_alloc(curr->next, size, align))) {
    return ret;
  }

  page_group_clear(page_groups, virt_to_phys((uint32_t)tmp.data), 1);
  memory_free(curr->next);
  curr->next = NULL;
  return NULL;
}

/*
  memory_page_data_alloc allocates a page for memory allocation. The first byte
  of a page used for memory allocation contains an empty memory map block.
*/
void* memory_page_data_alloc() {
  void* data = (void*)(phys_to_virt((uint32_t)page_group_alloc(page_groups, PHYS_OFFSET, 1, 1, 0)));

  struct initmem_block block = {
    sizeof(struct initmem_block),
    0,
    BLOCK_RW,
    NULL,
    NULL
  };

  block.begin += (uint32_t)data;
  *(struct initmem_block*)data = block;
  return data;
}

/*
  memory_block_page_alloc allocates a block of "size" bytes aligned to "align"
  bytes from the page information "page" and returns a pointer to it.
*/
void* memory_block_page_alloc(struct memory_page_info* page, size_t size, size_t align) {
  struct initmem_block* curr = (struct initmem_block*)page->data;
  struct initmem_block next;

  if (!size || size > PAGE_SIZE - sizeof(struct initmem_block)) {
    return NULL;
  }

  /*
    If this is a new page with an empty first memory map block, then use that
    block.
  */
  if (!curr->size) {
    uint32_t begin = ALIGN(curr->begin + curr->size, align);

    if (begin + size - 1 <= (uint32_t)page->data + PAGE_SIZE - 1) {
      curr->begin = begin;
      curr->size = size;
      return (void*)curr->begin;
    }
  }

  while (curr) {
    struct initmem_block* next_addr = (struct initmem_block*)(ALIGN(curr->begin + curr->size + sizeof(struct initmem_block), align) - sizeof(struct initmem_block));

    next.begin = (uint32_t)next_addr + sizeof(struct initmem_block);
    next.size = size;
    next.flags = BLOCK_RW;
    next.prev = curr;

    /* We allocate after the current block. */
    if (!curr->next) {
      if (next.begin + size >= (uint32_t)page->data + PAGE_SIZE) {
        return NULL;
      }

      next.next = NULL;
      *next_addr = next;
      curr->next = next_addr;
      return (uint32_t*)next.begin;
    }

    /* We allocate between the current block and the next block. */
    if (next.begin + size < (uint32_t)curr->next) {
      next.next = curr->next;
      *next_addr = next;
      curr->next = next_addr;
      return (uint32_t*)next.begin;
    }

    curr = curr->next;

  }

  return NULL;
}

/*
  memory_alloc allocates a naturally aligned block of size "size" bytes and
  returns a pointer to it.
*/
void* memory_alloc(size_t size) {
  if (!size) {
    return NULL;
  }
  else if (size <= PAGE_SIZE - sizeof(struct initmem_block)) {
    return memory_block_alloc(size);
  }
  else {
    return memory_page_alloc(page_count(size));
  }
}

/*
  memory_free frees the block which "ptr" points to.
*/
int memory_free(void* ptr) {
  struct initmem_block* block;

  if (!ptr) {
    return 0;
  }

  block = (struct initmem_block*)((uint32_t)ptr - sizeof(struct initmem_block));

  if (block->prev) {
    block->prev->next = block->next;
  }
  else if (block->next) {
    block->next->prev = block->prev;
  }

  /* Handle memory allocated by the page allocator. */
  if (block->size > PAGE_SIZE - sizeof(struct initmem_block)) {
    page_group_clear(page_groups, virt_to_phys((uint32_t)block->begin), block->size >> PAGE_SHIFT);
  }

  return 1;
}
