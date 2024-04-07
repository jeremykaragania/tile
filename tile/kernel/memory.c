#include <memory.h>

static struct memory_map_block memory_map_memory_block;
static struct memory_map_group memory_map_memory_group;

static struct memory_map_block memory_map_reserved_block;
static struct memory_map_group memory_map_reserved_group;

struct memory_map_info memory_map_info = {
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager_info init_memory_manager_info;

void init_memory_map_info() {
  memory_map_memory_block.begin = (uint32_t)&KERNEL_SPACE_PADDR;
  memory_map_memory_block.size = 0x80000000;
  memory_map_memory_group.length = 1;
  memory_map_memory_group.blocks = &memory_map_memory_block;

  memory_map_reserved_block.begin = (uint32_t)&text_begin;
  memory_map_reserved_block.size = (uint32_t)&bss_end - (uint32_t)&text_begin;
  memory_map_reserved_group.length = 1;
  memory_map_reserved_group.blocks = &memory_map_reserved_block;
  return;
}

void init_init_memory_manager_info(void* pg_dir, void* text_begin, void* text_end, void* data_begin, void* data_end) {
  init_memory_manager_info.pg_dir = (uint32_t*)phys_to_virt((uint32_t)pg_dir);
  init_memory_manager_info.text_begin = phys_to_virt((uint32_t)text_begin);
  init_memory_manager_info.text_end = phys_to_virt((uint32_t)text_end);
  init_memory_manager_info.data_begin = phys_to_virt((uint32_t)data_begin);
  init_memory_manager_info.data_end = phys_to_virt((uint32_t)data_end);
  return;
}
