#include <memory.h>

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

uint32_t high_memory;

/*
  init_memory_map initializes the kernel's memory map.
*/
void init_memory_map() {
  memory_map_add_block(memory_map.memory, KERNEL_SPACE_PADDR, 0x80000000);
  memory_map_add_block(memory_map.reserved, 0x80000000, 0x8000);
  memory_map_add_block(memory_map.reserved, PG_DIR_PADDR, PG_DIR_SIZE);
  memory_map_add_block(memory_map.reserved, (uint32_t)&text_begin, (uint32_t)&bss_end - (uint32_t)&text_begin);
}

/*
  init_memory_manager initializes the kernel's memory manager.
*/
void init_memory_manager(void* pg_dir, void* text_begin, void* text_end, void* data_begin, void* data_end) {
  memory_manager.pg_dir = (uint32_t*)phys_to_virt((uint32_t)pg_dir);
  memory_manager.text_begin = phys_to_virt((uint32_t)text_begin);
  memory_manager.text_end = phys_to_virt((uint32_t)text_end);
  memory_manager.data_begin = phys_to_virt((uint32_t)data_begin);
  memory_manager.data_end = phys_to_virt((uint32_t)data_end);
}

/*
  update_memory_map iterates over the memory map and invalidates blocks for
  memory allocation. It then finds the new bounds of low memory and high
  memory, which should be interpreted under the Linux meaning.
*/
void update_memory_map() {
  struct memory_map_block* b;
  uint64_t b_end;
  uint64_t vmalloc_begin_paddr = virt_to_phys(VMALLOC_BEGIN_VADDR);
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
      if (b->begin < vmalloc_begin_paddr) {
        if (b_end > vmalloc_begin_paddr) {
          lowmem_end = vmalloc_begin_paddr;
        }
        else {
          lowmem_end = b_end;
        }
      }
    }
  }

  high_memory = lowmem_end;
  memory_map.limit = lowmem_end;
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
  size_t overlap = 0;

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

      if (begin - 1 <= b_end) {
        ++overlap;
      }
    }

    memory_map_insert_block(group, pos, begin, size);
  }

  pos -= overlap;
  memory_map_merge_blocks(group, pos, group->size);
}

/*
  memory_alloc allocates a block of "size" bytes aligned to PAGE_SIZE and
  returns a pointer to it. Memory is allocated adjacent to reserved memory.
*/
void* memory_alloc(size_t size) {
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

      if (a_begin < r->begin && a_end < r_end) {
        memory_map_add_block(memory_map.reserved, a_begin, size);
        return (uint32_t*)a_begin;
      }

      a_begin = r_end + 1;
    }
  }

  /* We try to allocate after the ending reserved block. */
  a_begin = memory_map.reserved->blocks[rs-1].begin + memory_map.reserved->blocks[rs-1].size;
  a_end = a_begin + size - 1;

  if (a_end < m_end) {
    memory_map_add_block(memory_map.reserved, a_begin, size);
    return (uint32_t*)a_begin;
  }

  return NULL;
}

/*
  memory_free frees the block which "ptr" points to.
*/
int memory_free(void* ptr) {
  struct memory_map_block* block;

  for (size_t i = 0; i < memory_map.reserved->size; ++i) {
    block = &memory_map.reserved->blocks[i];

    if (block->begin == (uint32_t)ptr) {
      memmove(block, block + 1, (memory_map.reserved->size - i) * sizeof(*block));
      --memory_map.reserved->size;
      return 1;
    }
  }

  return 0;
}
