#ifndef PAGE_H
#define PAGE_H

#include <kernel/list.h>
#include <stdbool.h>
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
  struct file_info_int* file_int;
  struct list_link link;
};

/*
  enum page_region_flags represents the attributes of a virtual page region.
*/
enum page_region_flags {
  PAGE_READ = 0x1,
  PAGE_WRITE = 0x2,
  PAGE_EXECUTE = 0x4,
  PAGE_KERNEL = 0x8
};

/*
  struct descriptor_bits represents the bit indexes of certain descriptor
  protection bits. "ap" are the access permissions bits, and "xn" is the
  execute-never bit.
*/
struct descriptor_bits {
  uint8_t ap[3];
  uint8_t xn;
};

const extern uint32_t* vector_table_begin;
const extern uint32_t* vector_table_end;

extern void invalidate_entire_tlb();
extern void set_pgd(uint32_t pgd);

void init_paging();
void init_pgd();

void map_kernel();
void map_peripherals();
void map_vector_table();
void map_smc();

void* create_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void* create_section_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void* create_page_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags);
void remap_section(uint32_t* pmd, uint32_t pmd_page_table);
void* find_unmapped_region(uint32_t size);

uint32_t* addr_to_pmd(uint32_t* pgd, uint32_t addr);
uint32_t* addr_to_pte(uint32_t* pmd, uint32_t addr);
uint32_t pmd_to_addr(uint32_t* pgd, uint32_t* pmd);
uint32_t pmd_section_to_addr(uint32_t pmd);
uint32_t pte_to_addr(uint32_t pte);

void pmd_clear(uint32_t* pgd, uint32_t addr);
void pte_clear(uint32_t* pmd, uint32_t addr);
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint32_t p_addr, int flags);

bool is_pmd_page_table(uint32_t* pmd);
bool is_pmd_section(uint32_t* pmd);
uint32_t* pmd_to_page_table(uint32_t* pmd);

uint32_t* create_pgd();
uint32_t create_pmd_section(uint32_t p_addr, int flags);
uint32_t create_pmd_page_table(uint32_t* page_table);
uint32_t create_pte(uint32_t p_addr, int flags);
int get_descriptor_protection(uint32_t d, const struct descriptor_bits* bits);
uint32_t set_descriptor_protection(uint32_t d, const struct descriptor_bits* bits, int flags);

void create_page_region_bounds(struct list_link* head);
struct page_region* create_page_region(struct list_link* head, uint32_t begin, size_t count, int flags);
void insert_page_region(struct list_link* head, struct page_region* region);
void remove_page_region(struct list_link* head, struct page_region* region);
struct page_region* split_page_region(struct page_region* region, size_t index);
struct page_region* find_page_region(struct list_link* head, uint32_t addr);
void free_page_regions();

#endif
