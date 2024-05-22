#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <memory.h>

#define VMALLOC_BEGIN_VADDR 0xf0000000
#define VMALLOC_END_VADDR 0xff800000

extern uint32_t pgd_index(uint32_t addr);
extern uint32_t pte_index(uint32_t addr);
extern void invalidate_entire_tlb();

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pte_offset(uint32_t* pte, uint32_t addr);
uint32_t* pte_alloc(struct memory_manager* mm, uint32_t addr);
void pte_insert(uint32_t* pte, uint32_t v_addr, uint32_t p_addr);

void init_paging();

#endif
