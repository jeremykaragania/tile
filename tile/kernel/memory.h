#ifndef MEMORY_H
#define MEMORY_H

#define PHYS_OFFSET 0x80000000
#define VIRT_OFFSET 0xC0000000
#define TEXT_OFFSET 0x00008000
#define KERNEL_RAM_PADDR PHYS_OFFSET + TEXT_OFFSET
#define KERNEL_RAM_VADDR VIRT_OFFSET + TEXT_OFFSET

#include <stdint.h>

uint32_t virt_to_phys(uint32_t x);

uint32_t phys_to_virt(uint32_t x);

#endif
