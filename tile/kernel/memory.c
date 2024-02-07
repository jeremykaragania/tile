#include <memory.h>

uint32_t virt_to_phys(uint32_t x) {
  return x + (VIRT_OFFSET - PHYS_OFFSET);
}

uint32_t phys_to_virt(uint32_t x) {
  return x - (VIRT_OFFSET - PHYS_OFFSET);
}
