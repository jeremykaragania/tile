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
#include <kernel/process.h>
#include <lib/string.h>

static struct initmem_block initmem_memory_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct initmem_group initmem_memory_group;

static struct initmem_block initmem_reserved_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct initmem_group initmem_reserved_group;

struct initmem_info initmem_info;

struct page_group* page_groups;
struct list_link page_groups_head = LIST_INIT(page_groups_head);

struct list_link alloc_pages_head = LIST_INIT(alloc_pages_head);

uint32_t high_memory;

/*
  initmem_init initializes the initial memory allocator.
*/
void initmem_init() {
  struct memory_info* mem = init_process.mem;
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

  initmem_add_block(initmem_info.memory, PHYS_OFFSET, PHYS_SIZE);
  initmem_add_block(initmem_info.reserved, PHYS_OFFSET, TEXT_OFFSET);
  initmem_add_block(initmem_info.reserved, PG_DIR_PADDR, PG_DIR_SIZE);
  initmem_add_block(initmem_info.reserved, virt_to_phys(mem->text_begin), mem->bss_end - mem->text_begin);
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
      continue;
    }

    b_end = b->begin + b->size - 1;

    if (b_end > vmalloc_min_paddr) {
      lowmem_end = vmalloc_min_paddr;
      initmem_split_block(initmem_info.memory, lowmem_end);
      break;
    }
    else {
      lowmem_end = b_end;
    }
  }

  high_memory = phys_to_virt(lowmem_end);
  initmem_info.limit = lowmem_end;
  lowmem_end -= 1;
}

/*
  mem_init initializes the primary memory allocator.
*/
void memory_alloc_init() {
  struct initmem_block* block;
  struct page_group* group;
  struct list_link* curr;

  for (size_t i = 0; i < initmem_info.memory->size; ++i) {
    block = &initmem_info.memory->blocks[i];
    group = initmem_alloc(sizeof(struct page_group));

    group->pages = initmem_alloc((block->size >> PAGE_SHIFT) * sizeof(struct phys_page));
    group->size = block->size;
    group->offset = block->begin;

    page_group_insert(&page_groups_head, group);
  }

  curr = page_groups_head.next;

  /*
    For each memory block in the initial memory map's reserved group, we
    reserve that memory block in the appropriate physical page page_group.
  */
  for (size_t i = 0; i < initmem_info.reserved->size; ++i) {
    block = &initmem_info.reserved->blocks[i];

    group = list_data(curr, struct page_group, link);

    while (block->begin > page_group_end(group)) {
      curr = curr->next;
      group = list_data(curr, struct page_group, link);
    }

    page_group_reserve(group, block->begin, page_count(block->size));
  }

  page_groups = list_data(page_groups_head.next, struct page_group, link);
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
inline size_t page_group_index(const struct page_group* group, uint64_t addr) {
  return page_index(addr - group->offset);
}

/*
  page_group_to_addr returns the page address from a page group entry index
  "index" in the page group "group".
*/
inline uint64_t page_group_addr(const struct page_group* group, size_t index) {
  return page_addr(page_index(group->offset) + index);
}

/*
  page_group_end_addr returns the upper address bound which the page group
  "group" contains.
*/
inline uint64_t page_group_end(const struct page_group* group) {
  return group->offset + group->size - 1;
}

/*
  page_group_get returns the page information from a page group "group" and a
  page address "addr".
*/
inline struct phys_page* page_group_get(const struct page_group* group, uint64_t addr) {
  return &group->pages[page_group_index(group, addr)];
}

/*
  page_group_addr_is_free returns true if "count" contiguous pages are free
  from the address "addr" in the page group "group".
*/
bool page_group_is_free(const struct page_group* group, uint64_t addr, size_t count) {
  if (addr >= group->offset && addr + count * PAGE_SIZE <= page_group_end(group)) {
    int is_free = 1;

    for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
      is_free &= !(page_group_get(group, addr)->flags & PAGE_RESERVED);

      if (!is_free) {
        return false;
      }
    }
    return true;
  }

  return false;
}

/*
  page_group_reserve reserves "count" contiguous pages from the address "addr"
  in the page group "page_group".
*/
void page_group_reserve(struct page_group* group, uint64_t addr, size_t count) {
  for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
    page_group_get(group, addr)->flags |= PAGE_RESERVED;
  }
}

/*
  page_group_clear unreserves "count" contiguous pages from the address "addr"
  in the page group "page_group".
*/
void page_group_clear(struct page_group* group, uint64_t addr, size_t count) {
  for (size_t i = 0; i < count; ++i, addr += PAGE_SIZE) {
    page_group_get(group, addr)->flags &= ~PAGE_RESERVED;
  }
}

/*
  page_group_alloc allocates "count" contiguous pages aligned to "align" pages
  with "gap" free pages before it in the page group "group" between "begin" and
  "end". It returns its physical address.
*/
uint64_t page_group_alloc(struct page_group* group, uint64_t begin, uint64_t end, size_t count, size_t align, size_t gap) {
  uint64_t addr;
  uint32_t gap_size = gap << PAGE_SHIFT;

  for (size_t i = page_group_index(group, begin + gap_size); i < page_group_index(group, end); ++i) {
    addr = page_group_addr(group, i);

    if (!page_group_is_free(group, addr, count) || addr % (align << PAGE_SHIFT) != 0) {
      continue;
    }

    if (page_group_is_free(group, addr - gap_size, count)) {
      page_group_reserve(group, addr, count);
      return addr;
    }
  }

  return 0;
}

/*
  page_group_alloc_virt allocates "count" contiguous pages aligned to "align"
  pages with "gap" free pages before it in the page group "group" and returns a
  pointer to its virtual address.
*/
void* page_group_alloc_virt(struct page_group* group, size_t count, size_t align, size_t gap) {
  return (void*)phys_to_virt(page_group_alloc(group, PHYS_OFFSET, high_memory, count, align, gap));
}

/*
  page_group_insert inserts the page group "group" into the page group list
  specified by "head". It inserts the page such that the page group list
  remains sorted.
*/
int page_group_insert(struct list_link* head, struct page_group* group) {
  uint64_t group_begin;
  uint64_t group_end;
  uint64_t curr_group_begin;
  uint64_t curr_group_end;
  uint64_t next_group_begin;
  struct list_link* curr;
  struct page_group* curr_group;
  struct page_group* next_group;

  group_begin = group->offset;
  group_end = group->offset + group->size - 1;

  curr = head->next;

  do {
    curr_group = list_data(curr, struct page_group, link);
    next_group = list_data(curr->next, struct page_group, link);

    curr_group_begin = curr_group->offset;
    curr_group_end = curr_group_begin + curr_group->size - 1;
    next_group_begin = next_group->offset;

    /*
      Insert before current group.
    */
    if (group_end < curr_group_begin) {
      list_push(curr->prev, &group->link);
      return 0;
    }

    /*
      Insert after current group.
    */
    if (group_begin > curr_group_end && group_end < next_group_begin) {
      list_push(curr, &group->link);
      return 0;
    }

    curr = curr->next;
  } while (curr != head);

  return -1;
}

/*
  page_to_group finds and returns the page group in the page group list
  specified by head" containing the physical page "page".
*/
struct page_group* page_to_group(struct list_link* head, const struct phys_page* page) {
  struct page_group* group;
  struct list_link* curr = head->next;

  do {
    group = list_data(curr, struct page_group, link);

    if (page >= group->pages && page < group->pages + (group->size >> PAGE_SHIFT)) {
      return group;
    }

    curr = curr->next;
  } while (curr != head);

  return NULL;
}

/*
  page_to_addr returns the physical address of the physical page "page" given
  the page group list specified by "head".
*/
uint64_t page_to_addr(struct list_link* head, const struct phys_page* page) {
  struct page_group* group;
  uint64_t ret;

  group = page_to_group(head, page);

  if (!group) {
    return 0;
  }

  ret = group->offset + (page - group->pages) * PAGE_SIZE;

  return ret;
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
      return 0;
    }
  }

  return -1;
}

/*
  memory_page_alloc allocates "count" contiguous pages and returns a pointer to
  them.
*/
void* memory_page_alloc(size_t count) {
  void* data;
  struct phys_page* page;
  struct initmem_block* block;
  struct initmem_block* head;

  if (!count) {
    return NULL;
  }

  /*
    Make sure that the page has a free page before it to store page information
    inside.
  */
  data = page_group_alloc_virt(page_groups, count, count, 1);

  if (!data) {
    return NULL;
  }

  page_group_reserve(page_groups, virt_to_phys((uint32_t)data) - PAGE_SIZE, 1);

  head = (void*)((uint32_t)data - PAGE_SIZE);
  block = (void*)((uint32_t)data - sizeof(struct initmem_block));

  /* Initialize the block information to describe the page allocation. */
  block->begin = (uint32_t)data;
  block->size = count * PAGE_SIZE;
  block->next = NULL;
  block->prev = head;

  /* Initialize the dummy head block information to link to the previous block. */
  page = alloc_page_init(head);
  head->next = block;

  list_push(&alloc_pages_head, &page->link);

  return data;
}

/*
  memory_block_alloc allocates a memory region under a page size and returns a
  pointer to it.
*/
void* memory_block_alloc(size_t size) {
  struct phys_page* page;
  struct list_link* curr;
  void* data;
  void* ret;
  size_t align = 4;

  if (is_power_of_two(size)) {
    align = size;
  }

  curr = alloc_pages_head.next;

  /*
    We try to allocate in one of the existing pages.
  */
  while (curr != &alloc_pages_head) {
    page = list_data(curr, struct phys_page, link);

    if ((ret = memory_block_page_alloc(page, size, align))) {
      return ret;
    }

    curr = curr->next;
  }

  /*
    We couldn't allocate in one of the existing pages, so create a new
    allocation page.
  */
  data = page_group_alloc_virt(page_groups, 1, 1, 0);

  if (!data) {
    return NULL;
  }

  page = alloc_page_init(data);
  list_push(&alloc_pages_head, &page->link);

  if ((ret = memory_block_page_alloc(page, size, align))) {
    return ret;
  }

  page_group_clear(page_groups, virt_to_phys((uint32_t)data), 1);
  list_remove(&alloc_pages_head, &page->link);
  return NULL;
}

/*
  alloc_page_init initializes the physical page correlating to the virtual
  address specified by "data" for memory allocation and returns it.
*/
struct phys_page* alloc_page_init(void* data) {
  struct phys_page* page;
  struct initmem_block* block;

  page = page_group_get(page_groups, virt_to_phys((uint32_t)data));

  block = data;
  block->begin = (uint32_t)data + sizeof(struct initmem_block);
  block->size = 0;
  block->next = NULL;
  block->prev = NULL;

  return page;
}

/*
  memory_block_page_alloc allocates a block of "size" bytes aligned to "align"
  bytes from the page information "page" and returns a pointer to it.
*/
void* memory_block_page_alloc(struct phys_page* page, size_t size, size_t align) {
  void* page_data = (void*)phys_to_virt(page_to_addr(&page_groups_head, page));
  struct initmem_block* curr = (struct initmem_block*)page_data;
  struct initmem_block next;

  if (!size || size > MAX_BLOCK_SIZE) {
    return NULL;
  }

  next.size = size;

  while (curr) {
    struct initmem_block* next_addr = (struct initmem_block*)(ALIGN(curr->begin + curr->size + sizeof(struct initmem_block), align) - sizeof(struct initmem_block));

    next.begin = (uint32_t)next_addr + sizeof(struct initmem_block);
    next.prev = curr;

    /* We allocate after the current block. */
    if (!curr->next) {
      if (next.begin + size >= (uint32_t)page_data + PAGE_SIZE) {
        return NULL;
      }

      next.next = NULL;
      *next_addr = next;
      curr->next = next_addr;
      return (uint32_t*)next.begin;
    }

    /* We allocate between the current block and the next block. */
    if (next.begin + size <= (uint32_t)curr->next) {
      next.next = curr->next;
      *next_addr = next;
      curr->next->prev = next_addr;
      curr->next = next_addr;
      return (uint32_t*)next.begin;
    }

    curr = curr->next;

  }

  return NULL;
}

/*
  pages_alloc allocates "count" pages in the page zone "zone" and returns the
  first physical address.
*/
struct phys_page* pages_alloc(size_t count, int zone) {
  uint64_t addr;
  struct list_link* curr;
  struct page_group* group;
  uint64_t begin;
  uint64_t end;

  switch (zone) {
    case ZONE_LOWMEM:
      begin = PHYS_OFFSET;
      end = virt_to_phys(high_memory);
      break;
    case ZONE_HIGHMEM:
      begin = virt_to_phys(high_memory);
      end = PHYS_END;
      break;
    case ZONE_ANY:
      begin = PHYS_OFFSET;
      end = PHYS_END;
    default:
      break;
  }

  curr = page_groups_head.next;

  do {
    group = list_data(curr, struct page_group, link);

    if (group->offset < begin || page_group_end(group) > end) {
      curr = curr->next;
      continue;
    }

    addr = page_group_alloc(group, begin, end, count, count, 0);

    if (addr) {
      return page_group_get(group, addr);
    }

    curr = curr->next;
  } while(curr != &page_groups_head);

  return NULL;
}

/*
  pages_free frees "count" pages from the physical address "addr".
*/
void pages_free(struct phys_page* page, size_t count) {
  uint64_t addr;
  struct list_link* curr;
  struct page_group* group;

  addr = page_to_addr(&page_groups_head, page);
  curr = page_groups_head.next;

  do {
    group = list_data(curr, struct page_group, link);

    if (addr >= group->offset && addr < page_group_end(group)) {
      page_group_clear(group, addr, count);
      return;
    }

    curr = curr->next;
  } while(curr != &page_groups_head);
}

/*
  memory_alloc allocates a naturally aligned block of size "size" bytes and
  returns a pointer to it.
*/
void* memory_alloc(size_t size) {
  if (!size) {
    return NULL;
  }
  else if (size <= MAX_BLOCK_SIZE) {
    return memory_block_alloc(size);
  }
  else {
    return memory_page_alloc(page_count(size));
  }
}

/*
  memory_free frees the block which "ptr" points to.
*/
void memory_free(void* ptr) {
  struct initmem_block* block;
  struct phys_page* page;

  block = ptr_to_block(ptr);

  if (!ptr) {
    return;
  }

  if (block->prev) {
    block->prev->next = block->next;
  }

  if (block->next) {
    block->next->prev = block->prev;
  }

  /* Handle memory allocated by the page allocator. */
  if (block->size > MAX_BLOCK_SIZE) {
    page_group_clear(page_groups, virt_to_phys((uint32_t)block->begin), block->size >> PAGE_SHIFT);
  }

  /*
    If the allocation page is empty, then unreserve it and remove it from the
    allocation page list.
  */
  if (!block->prev->prev && !block->prev->next) {
    page = page_group_get(page_groups, virt_to_phys((uint32_t)block->prev));
    page_group_clear(page_groups, virt_to_phys((uint32_t)block->prev), 1);
    list_remove(&alloc_pages_head, &page->link);
  }
}
