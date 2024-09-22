#ifndef PAGE_H
#define PAGE_H

#include <kernel/mci.h>
#include <kernel/memory.h>
#include <kernel/uart.h>
#include <stdint.h>

#define VIRT_BITMAP_SIZE (VADDR_SPACE_SIZE / PAGE_SIZE / 32)

#define pgd_index_abs(addr) (addr >> PG_DIR_SHIFT)
#define pmd_index_abs(addr) ((addr & PAGE_MASK) >> PAGE_SHIFT)
#define pgd_index(addr) (4 * pgd_index_abs(addr))
#define pmd_index(addr) (4 * pmd_index_abs(addr))

extern uint32_t virt_bitmap[VIRT_BITMAP_SIZE];

extern void invalidate_entire_tlb();

void init_paging();
void init_pgd();

void map_kernel();
void map_vector_table();
void map_smc();

uint32_t* page_alloc(int flags);
int page_free(uint32_t* addr);

void create_mapping(uint32_t v_addr, uint64_t p_addr, uint32_t size, int flags);
int addr_is_mapped(uint32_t* addr);

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pmd_offset(uint32_t* pmd, uint32_t addr);
uint32_t* pmd_alloc(uint32_t* pgd, uint32_t addr);
void pmd_clear(uint32_t* pgd, uint32_t addr);
void pte_clear(uint32_t* pmd, uint32_t addr);
void virt_bitmap_clear(uint32_t addr);
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint64_t p_addr, int flags);
void virt_bitmap_insert(uint32_t v_addr, uint32_t size);

int pmd_is_page_table(uint32_t* pmd);
uint32_t* pmd_to_page_table(uint32_t* pmd);
uint32_t pmd_section_to_addr(uint32_t pmd);
uint32_t pte_to_addr(uint32_t pte);

uint32_t create_pmd_section(uint64_t p_addr, int flags);
uint32_t create_pmd_page_table(uint32_t* page_table);
uint32_t create_pte(uint64_t p_addr, int flags);

#endif
