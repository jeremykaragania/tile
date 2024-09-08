#ifndef PAGE_H
#define PAGE_H

#include <kernel/mci.h>
#include <kernel/memory.h>
#include <kernel/uart.h>
#include <stdint.h>

#define PAGE_BITMAP_SIZE (VADDR_SPACE_SIZE / PAGE_SIZE / 32)

#define pgd_index_abs(addr) (addr >> PG_DIR_SHIFT)
#define pmd_index_abs(addr) ((addr & PAGE_MASK) >> PAGE_SHIFT)
#define pgd_index(addr) (4 * pgd_index_abs(addr))
#define pmd_index(addr) (4 * pmd_index_abs(addr))
#define page_index(addr) (pgd_index_abs(addr) * PAGES_PER_PAGE_TABLE + pmd_index_abs(addr))
#define page_bitmap_index(addr) (page_index(addr) / 32)
#define page_bitmap_index_index(addr) (pmd_index_abs(addr) % 32)

extern uint32_t page_bitmap[PAGE_BITMAP_SIZE];

extern void invalidate_entire_tlb();

void init_paging();
void init_pgd();

void map_kernel();
void map_vector_table();
void map_smc();

void create_mapping(uint32_t v_addr, uint64_t p_addr, uint32_t size, int flags);
int mapping_exists(uint32_t* pgd, uint32_t v_addr, uint32_t p_addr);
uint32_t pgd_walk(uint32_t* pgd, uint32_t v_addr);

uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr);
uint32_t* pmd_offset(uint32_t* pmd, uint32_t addr);
uint32_t* pmd_alloc(uint32_t* pgd, uint32_t addr);
void pmd_clear(uint32_t* pgd, uint32_t addr);
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint64_t p_addr, int flags);
void page_bitmap_insert(uint32_t v_addr, uint32_t size);

int pmd_is_page_table(uint32_t* pmd);
uint32_t* pmd_to_page_table(uint32_t* pmd);
uint32_t pmd_section_to_addr(uint32_t pmd);
uint32_t pte_to_addr(uint32_t pte);

uint32_t create_pmd_section(uint64_t p_addr, int flags);
uint32_t create_pmd_page_table(uint32_t* page_table);
uint32_t create_pte(uint64_t p_addr, int flags);

#endif
