#include <memory.h>

struct memory_manager_info init_memory_manager_info = {
  (uint32_t*)&PG_DIR_VADDR,
  (uint32_t)&text_begin,
  (uint32_t)&text_end,
  (uint32_t)&data_begin,
  (uint32_t)&data_end
};

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
