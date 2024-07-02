#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>
#include <memory.h>
#include <uart.h>

extern uint32_t pgd_index(uint32_t addr);
extern uint32_t pmd_index(uint32_t addr);
extern void invalidate_entire_tlb();

void init_paging();
void init_pgd();

void map_kernel();
void map_vector_table();
void map_smc();

void create_mapping(uint32_t v_addr, uint64_t p_addr, uint32_t size, int flags);

int mapping_exists(struct memory_manager* mm, uint32_t v_addr, uint32_t p_addr);
uint32_t pgd_walk(struct memory_manager* mm, uint32_t v_addr);

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pmd_offset(uint32_t* pmd, uint32_t addr);
uint32_t* pmd_alloc(struct memory_manager* mm, uint32_t addr);
void pmd_clear(struct memory_manager* mm, uint32_t addr);
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint64_t p_addr, int flags);

int pmd_is_page_table(uint32_t* pmd);
uint32_t* pmd_to_page_table(uint32_t* pmd);
uint32_t pmd_section_to_addr(uint32_t pmd);
uint32_t pte_to_addr(uint32_t pte);

uint32_t create_pmd_section(uint64_t p_addr, int flags);
uint32_t create_pmd_page_table(uint32_t* page_table);
uint32_t create_pte(uint64_t p_addr, int flags);

#endif
