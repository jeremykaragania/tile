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
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager init_memory_manager;

void init_memory_map() {
  memory_map_add_block(memory_map.memory, KERNEL_SPACE_PADDR, 0x80000000);
  memory_map_add_block(memory_map.reserved, (uint32_t)&text_begin, (uint32_t)&bss_end - (uint32_t)&text_begin);
  memory_map_add_block(memory_map.reserved, PG_DIR_PADDR, PG_DIR_SIZE);
}

void init_init_memory_manager(void* pg_dir, void* text_begin, void* text_end, void* data_begin, void* data_end) {
  init_memory_manager.pg_dir = (uint32_t*)phys_to_virt((uint32_t)pg_dir);
  init_memory_manager.text_begin = phys_to_virt((uint32_t)text_begin);
  init_memory_manager.text_end = phys_to_virt((uint32_t)text_end);
  init_memory_manager.data_begin = phys_to_virt((uint32_t)data_begin);
  init_memory_manager.data_end = phys_to_virt((uint32_t)data_end);
  return;
}

void memory_map_merge_blocks(struct memory_map_group* group, int begin, int end) {
  size_t i;
  size_t merged = 0;
  struct memory_map_block* a = &group->blocks[begin];
  struct memory_map_block* b;
  size_t a_end;
  for (i = begin + 1; i < end; ++i) {
    a_end = a->begin + a->size - 1;
    b = &group->blocks[i];
    if (a_end >= b->begin) {
      a->size += (b->begin - a_end) + b->size;
      ++merged;
    }
    a = &group->blocks[i];
  }
  group->length -= merged;
}

void memory_map_insert_block(struct memory_map_group* group, int pos, uint32_t begin, uint32_t size) {
  struct memory_map_block* block = &group->blocks[pos];
  memmove(block + 1, block, (group->length - pos) * sizeof(*block));
  block->begin = begin;
  block->size = size;
  ++group->length;
}

void memory_map_add_block(struct memory_map_group* group, uint32_t begin, uint32_t size) {
  size_t i;
  struct memory_map_block *block;
  uint32_t block_end;
  size_t pos = group->length;
  if (group->length == 0) {
    memory_map_insert_block(group, pos, begin, size);
  }
  else {
    for (i = 0; i < group->length; ++i) {
      block = &group->blocks[i];
      block_end = block->begin + block->size - 1;
      if (begin <= block_end) {
        pos = i;
        if (begin >= block->begin) {
          ++pos;
        }
        break;
      }
    }
    memory_map_insert_block(group, pos, begin, size);
  }
  memory_map_merge_blocks(group, pos, group->length);
  return;
}

void* memory_alloc(size_t size) {
  size_t i;
  size_t j;
  uint32_t a_begin;
  uint32_t a_end;
  struct memory_map_block* m;
  uint32_t m_end;
  struct memory_map_block* r;
  uint32_t r_end;
  size_t reserved_length = memory_map.reserved->length;
  size_t memory_length = memory_map.memory->length;
  size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  for (i = 0; i < memory_length; ++i) {
    m = &memory_map.memory->blocks[i];
    m_end = m->begin + memory_map.memory->blocks[i].size - 1;
    a_begin = m->begin;
    for (j = 0; j < reserved_length; ++j) {
      a_end = a_begin + size - 1;
      r = &memory_map.reserved->blocks[j];
      r_end = r->begin + r->size - 1;
      if (a_begin < r->begin && a_end < r_end) {
        memory_map_add_block(memory_map.reserved, a_begin, size);
        return (uint32_t*)a_begin;
      }
      if (j == reserved_length - 1) {
        a_begin = memory_map.reserved->blocks[j].begin + memory_map.reserved->blocks[j].size;
        a_end = a_begin + size;
        if (a_end < m_end) {
          memory_map_add_block(memory_map.reserved, a_begin, size);
          return (uint32_t*)a_begin;
        }
      }
      a_begin = r_end + 1;
    }
  }
  return NULL;
}

int memory_free(void* ptr) {
  size_t i;
  struct memory_map_block* block;
  for (i = 0; i < memory_map.reserved->length; ++i) {
    block = &memory_map.reserved->blocks[i];
    if (block->begin == (uint32_t)ptr) {
      memmove(block, block + 1, (memory_map.reserved->length - i) * sizeof(*block));
      --memory_map.reserved->length;
      return 1;
    }
  }
  return 0;
}
