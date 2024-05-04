#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
extern uint32_t pgd_index(uint32_t addr);

#endif
