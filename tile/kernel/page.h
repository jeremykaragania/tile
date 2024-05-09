#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

extern uint32_t pgd_index(uint32_t addr);
extern uint32_t pte_index(uint32_t addr);
extern void invalidate_entire_tlb();

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pte_offset(uint32_t* pte, uint32_t addr);

#endif
