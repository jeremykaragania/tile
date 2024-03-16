#include <memory.h>

uint32_t virt_to_phys(uint32_t x) {
  return x - ((uint32_t)&VIRT_OFFSET - (uint32_t)&PHYS_OFFSET);
}

uint32_t phys_to_virt(uint32_t x) {
  return x + ((uint32_t)&VIRT_OFFSET - (uint32_t)&PHYS_OFFSET);
}
