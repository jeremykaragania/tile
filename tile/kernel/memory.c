#include <memory.h>

static struct memory_map_block memory_map_memory_block[MEMORY_MAP_GROUP_LENGTH];
static struct memory_map_group memory_map_memory_group = {
  0,
  memory_map_memory_block
};

static struct memory_map_block memory_map_reserved_block[MEMORY_MAP_GROUP_LENGTH];
static struct memory_map_group memory_map_reserved_group = {
  0,
  memory_map_reserved_block
};

struct memory_map memory_map = {
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager init_memory_manager;

void init_memory_map() {
  memory_map_add_block(memory_map.memory, (uint32_t)&KERNEL_SPACE_PADDR, 0x80000000);
  memory_map_add_block(memory_map.reserved, (uint32_t)&text_begin, (uint32_t)&bss_end - (uint32_t)&text_begin);
  memory_map_add_block(memory_map.reserved, (uint32_t)&PG_DIR_PADDR, (uint32_t)&PG_DIR_SIZE);
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
  struct memory_map_block* a = &group->blocks[0];
  struct memory_map_block* b;
  size_t a_end;
  for (i = 1; i < group->length; ++i) {
    a_end = a->begin + a->size;
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
  size_t pos = 0;
  if (group->length == 0) {
    memory_map_insert_block(group, pos, begin, size);
  }
  else {
    for (i = 0; i < group->length; ++i) {
      block = &group->blocks[i];
      block_end = block->begin + block->size;
      if (begin <= block_end) {
        pos = i;
        if (begin >= block->begin) {
          ++pos;
        }
        memory_map_insert_block(group, pos, begin, size);
        break;
      }
    }
  }
  memory_map_merge_blocks(group, pos, group->length);
  return;
}
