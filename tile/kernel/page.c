#include <page.h>

/*
  pgd_offset returns the address of a page table entry from a virtual address
  "addr", and the base address of a page global directory "pgd". In ARMv7-A
  parlance, it returns the address of a page table first-level descriptor from
  a virtual address "addr", and a translation table base, "pgd".
*/
uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr) {
  return (uint32_t*)((uint32_t)pgd + (uint32_t)pgd_index(addr));
}

/*
  pte_offset returns the address of a page from a virtual address "addr", and
  the base address of a page table entry "pte". In ARMv7-A parlance, it returns
  the address of a small page second-level descriptor from a virtual address,
  "addr" and a page table base address, "pte".
*/
uint32_t* pte_offset(uint32_t* pte, uint32_t addr) {
  return (uint32_t*)((uint32_t)pte + (uint32_t)pte_index(addr));
}

/*
  pte_alloc allocates a new page table entry from a virtual address "addr" and
  returns a pointer to it. The appropriate index in the page global directory
  specified by the memory manager "mm" is updated to point to the new page
  table entry.
*/
uint32_t* pte_alloc(struct memory_manager* mm, uint32_t addr) {
  uint32_t* pte;
  uint32_t* offset;
  pte = memory_alloc(PAGE_SIZE);
  offset = pgd_offset(mm->pg_dir, addr);
  *offset = ((uint32_t)pte & 0xfffffc00) | 0x1;
  invalidate_entire_tlb();
  return pte;
}

/*
  pte_insert inserts a page into a page table entry "pte". The page is
  specified by which physical address "p_addr" it maps to which virtual address
  "v_addr". If a page already exists it is replaced.
*/
void pte_insert(uint32_t* pte, uint32_t v_addr, uint32_t p_addr) {
  uint32_t* offset;
  offset = pte_offset(pte, v_addr);
  *offset = ((uint32_t)p_addr & 0xfffff000) | 0x1 << 0x4 | 0x1 << 0x1;
  invalidate_entire_tlb();
}

void init_paging() {
  struct memory_map_block* b;
  uint32_t b_end;

  for (size_t i = 0; i < memory_map.memory->size; ++i) {
    b = &memory_map.memory->blocks[i];
    if (!IS_ALIGNED(b->begin, PAGE_SIZE)) {
      memory_map_mask_block(b, BLOCK_RESERVED, 1);
    }
  }
}
