#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

const extern uint32_t* PHYS_OFFSET;

const extern uint32_t* VIRT_OFFSET;

uint32_t virt_to_phys(uint32_t x);

uint32_t phys_to_virt(uint32_t x);

#endif
