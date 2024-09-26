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
  0xffffffffffffffffull,
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager memory_manager;

struct memory_bitmap phys_bitmaps;

uint32_t high_memory;

/*
  virt_to_phys returns a physical address from a virtual address "x".
*/
uint64_t virt_to_phys(uint32_t x) {
  return x - ((uint32_t)&VIRT_OFFSET - (uint32_t)&PHYS_OFFSET);
}

/*
  phys_to_virt returns a virtual address from a physical address "x".
*/
uint32_t phys_to_virt(uint64_t x) {
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
  for (uint64_t i = 0; i < memory_map.memory->size; ++i) {
    curr->size = memory_map.memory->blocks[i].size / PAGE_SIZE / 32;
    curr->data = memory_alloc(curr->size);
    curr->offset = memory_map.memory->blocks[i].begin;

    for (uint64_t j = 0; j < curr->size; ++j) {
      curr->data[j] = 0;
    }

    if (i + 1 > memory_map.memory->size) {
      curr->next = memory_alloc(sizeof(struct memory_bitmap));
    }

    curr = curr->next;
  }

  curr = &phys_bitmaps;

  /*
    For each memory block in the initial memory map's reserved group, we
    reserve that memory block in the appropriate physical page bitmap.
  */
  for (uint64_t i = 0; i < memory_map.reserved->size; ++i) {
    while (memory_map.reserved->blocks[i].begin > curr->offset + (curr->size * PAGE_SIZE * 32)) {
      curr = curr->next;
    }

    bitmap_insert(curr, memory_map.reserved->blocks[i].begin, memory_map.reserved->blocks[i].size);
  }
}

/*
  init_memory_manager initializes the kernel's memory manager.
*/
void init_memory_manager(void* pgd, void* text_begin, void* text_end, void* data_begin, void* data_end, void* bss_begin, void* bss_end) {
  memory_manager.pgd= (uint32_t*)(uint32_t)pgd;
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
  uint64_t b_end;
  uint64_t vmalloc_min_paddr = virt_to_phys(VMALLOC_MIN_VADDR);
  uint32_t lowmem_end = 0;

  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    b = &memory_map.memory->blocks[i];

    if (!IS_ALIGNED(b->begin, PAGE_SIZE)) {
      memory_map_mask_block(b, BLOCK_RESERVED, 1);
    }
  }

  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    b_end = b->begin + b->size;

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
void memory_map_insert_block(struct memory_map_group* group, int pos, uint64_t begin, uint64_t size) {
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
void memory_map_add_block(struct memory_map_group* group, uint64_t begin, uint64_t size) {
  struct memory_map_block *b;
  uint64_t b_end;
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
int memory_map_split_block(struct memory_map_group* group, uint64_t begin) {
  struct memory_map_block* a;
  uint64_t a_end;

  for (size_t i = 0; i < group->size; ++i) {
    a = &group->blocks[i];
    a_end = a->begin + a->size;

    if (begin > a->begin && begin < a_end) {
      uint64_t new_end;

      a->size = begin - a->begin;
      new_end = a->begin + a->size;
      memory_map_add_block(group, new_end, a_end - new_end);
      return i;
    }
  }

  return -1;
}

/*
  bitmap_index returns the bitmap entry index for the address "addr" in the
  bitmap "bitmap".
*/
size_t bitmap_index(const struct memory_bitmap* bitmap, uint64_t addr) {
  return (addr - bitmap->offset) / PAGE_SIZE / 32;
}

/*
  bitmap_index_index returns the bit index for the address "addr" in a bitmap
  entry in the bitmap "bitmap".
*/
size_t bitmap_index_index(const struct memory_bitmap* bitmap, uint64_t addr) {
  return (addr - bitmap->offset) / PAGE_SIZE % 32;
}

/*
  bitmap_to_addr returns the address from a bitmap entry index "i" and a bit
  index "j" in the bitmap "bitmap".
*/
uint64_t bitmap_to_addr(const struct memory_bitmap* bitmap, uint64_t i, uint64_t j) {
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
  memory_phys_alloc allocates a block of "size" bytes and returns a pointer to
  its physical address. Memory is allocated adjacent to reserved memory.
*/
void* memory_phys_alloc(size_t size) {
  uint64_t a_begin;
  uint64_t a_end;
  struct memory_map_block* m;
  uint64_t m_end;
  struct memory_map_block* r;
  uint64_t r_end;
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
        return (uint64_t*)(uint32_t)a_begin;
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
    return (uint64_t*)(uint32_t)a_begin;
  }

  return NULL;
}

/*
  memory_alloc allocates a block of "size" bytes and returns a pointer to its
  virtual address. Memory is allocated adjacent to reserved memory.
*/
void* memory_alloc(size_t size) {
  return (uint32_t*)phys_to_virt((uint32_t)memory_phys_alloc(size));
}

/*
  memory_free frees the block which "ptr" points to.
*/
int memory_free(void* ptr) {
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
