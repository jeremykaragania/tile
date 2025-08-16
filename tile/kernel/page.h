#ifndef PAGE_H
#define PAGE_H

#include <kernel/list.h>
#include <stddef.h>
#include <stdint.h>

#define pmd_count(x) (((x) + PMD_SIZE - 1) >> PG_DIR_SHIFT)
#define pgd_index_abs(addr) (addr >> PG_DIR_SHIFT)
#define pmd_index_abs(addr) ((addr & PAGE_MASK) >> PAGE_SHIFT)
#define pgd_index(addr) (4 * pgd_index_abs(addr))
#define pmd_index(addr) (4 * pmd_index_abs(addr))
#define page_region_end(region) (region->begin + PAGE_SIZE * region->count)

/*
  struct page_region represents a region of virtual pages. It differs from a
  struct page group as the pages it tracks are sparse not individual pages
  themselves.
*/
struct page_region {
  uint32_t begin;
  size_t count;
  int flags;
  struct list_link link;
};

const extern uint32_t* vector_table_begin;
const extern uint32_t* vector_table_end;

extern void invalidate_entire_tlb();
extern void set_pgd(uint32_t* pgd);

void init_paging();
void init_pgd();

void map_kernel();
void map_peripherals();
void map_vector_table();
void map_smc();

void* create_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void* create_section_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void* create_page_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void* find_unmapped_region(uint32_t size);

uint32_t* addr_to_pmd(uint32_t* pgd, uint32_t addr);
uint32_t* addr_to_pte(uint32_t* pmd, uint32_t addr);
uint32_t pmd_to_addr(uint32_t* pgd, uint32_t* pmd);
uint32_t pmd_section_to_addr(uint32_t pmd);
uint32_t pte_to_addr(uint32_t pte);

void pmd_clear(uint32_t* pgd, uint32_t addr);
void pte_clear(uint32_t* pmd, uint32_t addr);
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint32_t p_addr, int flags);

int pmd_is_page_table(uint32_t* pmd);
uint32_t* pmd_to_page_table(uint32_t* pmd);

void* create_pgd();
uint32_t create_pmd_section(uint32_t p_addr, int flags);
uint32_t create_pmd_page_table(uint32_t* page_table);
uint32_t create_pte(uint32_t p_addr, int flags);

struct page_region* create_page_region(struct list_link* head, uint32_t begin, size_t count, int flags);
void insert_page_region(struct list_link* head, struct page_region* region);
void remove_page_region(struct list_link* head, struct page_region* region);
struct page_region* split_page_region(struct page_region* region, size_t index);
struct page_region* find_page_region(struct list_link* head, uint32_t addr);

#endif
