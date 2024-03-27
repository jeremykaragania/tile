#include <memory.h>

static struct memory_region reserved_memory_region = {
  0x0,
  NULL
};

static struct memory_block memory_memory_blocks = {
  0x80000000,
  0x80000000
};

static struct memory_region memory_memory_region = {
  0x80000000,
  &memory_memory_blocks
};

struct memory_info memory_info = {
  &reserved_memory_region,
  &memory_memory_region
};
