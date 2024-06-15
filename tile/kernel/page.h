#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <memory.h>
#include <uart.h>

extern uint32_t pgd_index(uint32_t addr);
extern uint32_t pte_index(uint32_t addr);
extern void invalidate_entire_tlb();

void init_paging();
void init_pgd();

void map_smc();
void map_kernel();

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pte_offset(uint32_t* pte, uint32_t addr);
uint32_t* pte_alloc(struct memory_manager* mm, uint32_t addr);
void pte_clear(struct memory_manager* mm, uint32_t addr);
void pte_insert(uint32_t* pte, uint32_t v_addr, uint64_t p_addr);

#endif
