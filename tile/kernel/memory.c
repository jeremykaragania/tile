#include <kernel/memory.h>

static struct memory_map_block memory_map_memory_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct memory_map_group memory_map_memory_group = {
  0,
  memory_map_memory_blocks
};

static struct memory_map_block memory_map_reserved_blocks[MEMORY_MAP_GROUP_LENGTH];
static struct memory_map_group memory_map_reserved_group = {
  0,
  memory_map_reserved_blocks
};

struct memory_map memory_map = {
  0xffffffff,
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager memory_manager;

struct memory_bitmap phys_bitmaps;

struct memory_page_info memory_page_info_cache = {
  NULL,
  NULL
};

uint32_t high_memory;

/*
  virt_to_phys returns a physical address from a virtual address "x".
*/
uint32_t virt_to_phys(uint32_t x) {
  return x - ((uint32_t)&VIRT_OFFSET - (uint32_t)&PHYS_OFFSET);
}

/*
  phys_to_virt returns a virtual address from a physical address "x".
*/
uint32_t phys_to_virt(uint32_t x) {
  return x + ((uint32_t)&VIRT_OFFSET - (uint32_t)&PHYS_OFFSET);
}

/*
  init_memory_map initializes the kernel's memory map.
*/
void init_memory_map() {
  struct memory_bitmap* curr;

  /*
    We initialize the kernel's initial memory map using the system's memory
    map; and the kernel's memory map from the linker. Eventually we won't need
    this memory map, we just need it now to allocate the physical page bitmaps.
  */
  memory_map_add_block(memory_map.memory, KERNEL_SPACE_PADDR, 0x80000000);
  memory_map_add_block(memory_map.reserved, 0x80000000, 0x8000);
  memory_map_add_block(memory_map.reserved, PG_DIR_PADDR, PG_DIR_SIZE);
  memory_map_add_block(memory_map.reserved, virt_to_phys((uint32_t)&text_begin), (uint32_t)&bss_end - (uint32_t)&text_begin);

  curr = &phys_bitmaps;

  /*
    For each memory block in the initial memory map's memory group, we allocate
    a physical page bitmap to map that memory block.
  */
  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    curr->size = memory_map.memory->blocks[i].size / PAGE_SIZE / 32;
    curr->data = memory_map_alloc(curr->size);
    curr->offset = memory_map.memory->blocks[i].begin;

    for (size_t j = 0; j < curr->size; ++j) {
      curr->data[j] = 0;
    }

    if (i + 1 > memory_map.memory->size) {
      curr->next = memory_map_alloc(sizeof(struct memory_bitmap));
    }

    curr = curr->next;
  }

  curr = &phys_bitmaps;

  /*
    For each memory block in the initial memory map's reserved group, we
    reserve that memory block in the appropriate physical page bitmap.
  */
  for (size_t i = 0; i < memory_map.reserved->size; ++i) {
    while (memory_map.reserved->blocks[i].begin > curr->offset + curr->size * PAGE_SIZE * 32 - 1) {
      curr = curr->next;
    }

    bitmap_insert(curr, memory_map.reserved->blocks[i].begin, memory_map.reserved->blocks[i].size);
  }
}

/*
  init_memory_manager initializes the kernel's memory manager.
*/
void init_memory_manager(void* pgd, void* text_begin, void* text_end, void* data_begin, void* data_end, void* bss_begin, void* bss_end) {
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
  struct memory_map_block* b;
  uint32_t b_end;
  uint32_t vmalloc_min_paddr = virt_to_phys(VMALLOC_MIN_VADDR);
  uint32_t lowmem_end = 0;

  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    b = &memory_map.memory->blocks[i];

    if (!IS_ALIGNED(b->begin, PAGE_SIZE)) {
      memory_map_mask_block(b, BLOCK_RESERVED, 1);
    }
  }

  for (size_t i = 0; i < memory_map.memory->size; ++i) {
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
  memory_map.limit = lowmem_end;
  memory_map_split_block(memory_map.memory, memory_map.limit);
  lowmem_end -= 1;
}

/*
  memory_map_mask_block sets a flag "flag" to "mask" in "block".
*/
void memory_map_mask_block(struct memory_map_block* block, int flag, int mask) {
  if (mask) {
    block->flags |= flag;
  }
  else {
    block->flags &= ~flag;
  }
}

/*
  memory_map_merge_blocks merges the blocks in "group" from "begin" to "end".
*/
void memory_map_merge_blocks(struct memory_map_group* group, int begin, int end) {
  size_t merged = 0;
  struct memory_map_block* a = &group->blocks[begin];
  struct memory_map_block* b;
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
  memory_map_insert_block inserts a block into "group" at "pos". The block is
  specified by "begin" and "size".
*/
void memory_map_insert_block(struct memory_map_group* group, int pos, uint32_t begin, uint32_t size) {
  struct memory_map_block* block = &group->blocks[pos];

  memmove(block + 1, block, (group->size - pos) * sizeof(*block));
  block->begin = begin;
  block->size = size;
  ++group->size;
}

/*
  memory_map_add_block adds a block into "group". The block is specified by
  "begin" and "size". The block is added such that the blocks in "group" remain
  sorted by their beginning.
*/
void memory_map_add_block(struct memory_map_group* group, uint32_t begin, uint32_t size) {
  struct memory_map_block *b;
  uint32_t b_end;
  size_t pos = group->size;

  if (group->size == 0) {
    memory_map_insert_block(group, pos, begin, size);
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

    memory_map_insert_block(group, pos, begin, size);
  }

  memory_map_merge_blocks(group, 0, group->size);
}

/*
  memory_map_split tries to split a block in "group" at "begin" into two
  discrete blocks: a left block and a right block. It returns the index of the
  left block in the group if successful.
*/
int memory_map_split_block(struct memory_map_group* group, uint32_t begin) {
  struct memory_map_block* a;
  uint32_t a_end;

  for (size_t i = 0; i < group->size; ++i) {
    a = &group->blocks[i];
    a_end = a->begin + a->size - 1;

    if (begin > a->begin && begin < a_end) {
      uint32_t new_end;

      a->size = begin - a->begin;
      new_end = a->begin + a->size - 1;
      memory_map_add_block(group, new_end + 1, a_end - new_end);
      return i;
    }
  }

  return -1;
}

/*
  bitmap_index returns the bitmap entry index for the address "addr" in the
  bitmap "bitmap".
*/
size_t bitmap_index(const struct memory_bitmap* bitmap, uint32_t addr) {
  return (addr - bitmap->offset) / PAGE_SIZE / 32;
}

/*
  bitmap_index_index returns the bit index for the address "addr" in a bitmap
  entry in the bitmap "bitmap".
*/
size_t bitmap_index_index(const struct memory_bitmap* bitmap, uint32_t addr) {
  return (addr - bitmap->offset) / PAGE_SIZE % 32;
}

/*
  bitmap_to_addr returns the address from a bitmap entry index "i" and a bit
  index "j" in the bitmap "bitmap".
*/
uint32_t bitmap_to_addr(const struct memory_bitmap* bitmap, size_t i, size_t j) {
  return bitmap->offset + (i * PAGE_SIZE * 32 + PAGE_SIZE * j);
}

/*
  bitmap_insert inserts pages from the address "addr" spanning "size" bytes in
  the bitmap "bitmap".
*/
void bitmap_insert(struct memory_bitmap* bitmap, uint32_t addr, uint32_t size) {
  for (uint32_t i = addr; i < addr + size; i += PAGE_SIZE) {
    bitmap->data[bitmap_index(bitmap, i)] |= 1 << bitmap_index_index(bitmap, i);
  }
}

/*
  bitmap_clear clears a page from the address "addr" in the bitmap "bitmap".
*/
void bitmap_clear(struct memory_bitmap* bitmap, uint32_t addr) {
  bitmap->data[bitmap_index(bitmap, addr)] &= ~(1 << bitmap_index_index(bitmap, addr));
}

/*
  bitmap_alloc allocates a page in the bitmap "bitmap" above "begin" and
  returns a pointer to it.
*/
void* bitmap_alloc(struct memory_bitmap* bitmap, uint32_t begin) {
  void* addr = NULL;

  while (bitmap) {
    for (size_t i = bitmap_index(bitmap, begin); i < bitmap->size; ++i) {
      for (size_t j = 0; j < 32; ++j) {
        /* The page is free. */
        if (!(bitmap->data[i] & (1 << j))) {
          addr = (uint32_t*)bitmap_to_addr(bitmap, i, j);
          bitmap_insert(bitmap, (uint32_t)addr, PAGE_SIZE);
          return addr;
        }
      }
    }

    bitmap = bitmap->next;
  }

  return NULL;
}

/*
  memory_map_phys_alloc allocates a block of "size" bytes and returns a pointer
  to its physical address. Memory is allocated adjacent to reserved memory.
*/
void* memory_map_phys_alloc(size_t size) {
  uint32_t a_begin;
  uint32_t a_end;
  struct memory_map_block* m;
  uint32_t m_end;
  struct memory_map_block* r;
  uint32_t r_end;
  size_t rs = memory_map.reserved->size;

  /*
    We allocate a block with the constraints: "a" is the block which we are
    trying to allocate; "m" is the block where we can allocate; and "r" is the
    block where we can't allocate.
  */
  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    m = &memory_map.memory->blocks[i];
    m_end = m->begin + memory_map.memory->blocks[i].size - 1;
    a_begin = m->begin;
    a_end = a_begin + size - 1;

    if (a_end > memory_map.limit) {
      break;
    }

    for (size_t j = 0; j < memory_map.reserved->size; ++j) {
      r = &memory_map.reserved->blocks[j];
      r_end = r->begin + r->size - 1;

      if (a_end < r->begin) {
        memory_map_add_block(memory_map.reserved, a_begin, size);
        return (void*)a_begin;
      }

      a_begin = r_end + 1;
      a_end = a_begin + size - 1;
    }
  }

  /* We try to allocate after the ending reserved block. */
  a_begin = memory_map.reserved->blocks[rs-1].begin + memory_map.reserved->blocks[rs-1].size;
  a_end = a_begin + size - 1;

  if (a_end < m_end) {
    memory_map_add_block(memory_map.reserved, a_begin, size);
    return (void*)a_begin;
  }

  return NULL;
}

/*
  memory_map_alloc allocates a block of "size" bytes and returns a pointer to
  its virtual address. Memory is allocated adjacent to reserved memory.
*/
void* memory_map_alloc(size_t size) {
  return (uint32_t*)phys_to_virt((uint32_t)memory_map_phys_alloc(size));
}

/*
  memory_map_free frees the block which "ptr" points to.
*/
int memory_map_free(void* ptr) {
  struct memory_map_block* block;

  for (size_t i = 0; i < memory_map.reserved->size; ++i) {
    block = &memory_map.reserved->blocks[i];

    if (block->begin == virt_to_phys((uint32_t)ptr)) {
      memmove(block, block + 1, (memory_map.reserved->size - i) * sizeof(*block));
      --memory_map.reserved->size;
      return 1;
    }
  }

  return 0;
}

/*
  memory_page_info_data_alloc allocates a page for memory allocation. The first
  byte of a page used for memory allocation contains an empty memory map block.
*/
void* memory_page_info_data_alloc() {
  char* data = (char*)phys_to_virt((uint32_t)bitmap_alloc(&phys_bitmaps, phys_bitmaps.offset));
  struct memory_map_block block = {
    sizeof(struct memory_map_block),
    0,
    0,
    NULL,
    NULL
  };

  block.begin += (uint32_t)data;
  *(struct memory_map_block*)data = block;
  return data;
}

/*
  memory_alloc_page allocates a block of "size" bytes aligned to "align" bytes
  from the page information "page" and returns a pointer to it.
*/
void* memory_alloc_page(struct memory_page_info* page, size_t size, size_t align) {
  struct memory_map_block* curr = (struct memory_map_block*)page->data;
  uint32_t distance = 0;

  if (!size || size > PAGE_SIZE - sizeof(struct memory_map_block)) {
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
    /* We allocate after the current block. */
    if (!curr->next) {
      struct memory_map_block* next = (struct memory_map_block*)(ALIGN(curr->begin + curr->size + sizeof(struct memory_map_block), align) - sizeof(struct memory_map_block));
      uint32_t next_begin = (uint32_t)next + sizeof(struct memory_map_block);

      if (next_begin + size >= (uint32_t)page->data + PAGE_SIZE) {
        break;
      }

      curr->next = next;
      curr->next->begin = next_begin;
      curr->next->size = size;
      curr->next->next = NULL;
      curr->next->prev = curr;
      return (uint32_t*)curr->next->begin;
    }

    distance = (uint32_t)curr->next - (curr->begin + curr->size);

    /* We allocate between the current block and the next block. */
    if (distance >= size + sizeof(struct memory_map_block)) {
      struct memory_map_block* next = curr->next;

      curr->next = (struct memory_map_block*)(curr->begin + curr->size);
      curr->next->begin = (uint32_t)curr->next + sizeof(struct memory_map_block);
      curr->next->size = size;
      curr->next->next = next;
      curr->next->prev = curr;
      return (uint32_t*)curr->next->begin;
    }

    curr = curr->next;
  }

  return NULL;
}

/*
  memory_alloc allocates a block of size "size" bytes aligned to "align" bytes
  and returns a pointer to it.
*/
void* memory_alloc(size_t size, size_t align) {
  struct memory_page_info* curr = &memory_page_info_cache;
  struct memory_page_info tmp;
  void* ret = NULL;

  if (!size || size > PAGE_SIZE - sizeof(struct memory_map_block)) {
    return NULL;
  }

  /*
    We try to allocate in one of the existing pages.
  */
  while (curr) {
    if (!curr->data) {
      curr->data = memory_page_info_data_alloc();
    }

    ret = memory_alloc_page(curr, size, align);

    if (ret) {
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
  tmp.data = memory_page_info_data_alloc();
  tmp.next = NULL;
  curr->next = memory_alloc_page(&tmp, sizeof(struct memory_page_info), 1);
  curr->next->data = tmp.data;
  curr->next->next = NULL;

  ret = memory_alloc_page(curr->next, size, align);

  if (ret) {
    return ret;
  }

  return NULL;
}

/*
  memory_free frees the block which "ptr" points to.
*/
int memory_free(void* ptr) {
  struct memory_map_block* block = (struct memory_map_block*)((uint32_t)ptr - sizeof(struct memory_map_block));

  if (block->prev) {
    block->prev->next = block->next;
  }

  block->next = NULL;
  return 1;
}
