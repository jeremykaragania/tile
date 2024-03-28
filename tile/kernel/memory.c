#include <memory.h>

static struct memory_region reserved_memory_region = {
  0x0,
  NULL
};

static struct memory_block memory_memory_blocks = {
  (uint32_t)&KERNEL_SPACE_PADDR,
  0x80000000
};

static struct memory_region memory_memory_region = {
  0x1,
  &memory_memory_blocks
};

struct memory_info memory_info = {
  &memory_memory_region,
  &reserved_memory_region
};
