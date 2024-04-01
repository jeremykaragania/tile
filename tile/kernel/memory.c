#include <memory.h>

static struct memory_map_group memory_map_reserved_group = {
  0x0,
  NULL
};

static struct memory_map_block memory_map_memory_block = {
  (uint32_t)&KERNEL_SPACE_PADDR,
  0x80000000
};

static struct memory_map_group memory_map_memory_group = {
  0x1,
  &memory_map_memory_block
};

struct memory_map_info memory_map_info = {
  &memory_map_memory_group,
  &memory_map_reserved_group
};

struct memory_manager_info init_memory_manager_info;

void init_init_memory_manager_info(void* pg_dir, void* text_begin, void* text_end, void* data_begin, void* data_end) {
  init_memory_manager_info.pg_dir = pg_dir;
  init_memory_manager_info.text_begin = phys_to_virt((uint32_t)text_begin);
  init_memory_manager_info.text_end = phys_to_virt((uint32_t)text_end);
  init_memory_manager_info.data_begin = phys_to_virt((uint32_t)data_begin);
  init_memory_manager_info.data_end = phys_to_virt((uint32_t)data_end);
  return;
}
