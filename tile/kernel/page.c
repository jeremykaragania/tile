#include <page.h>

/*
  init_paging initializes the kernel's paging and maps required memory regions.
*/
void init_paging() {
  init_pgd();
  map_smc();
}

/*
  init_pgd initializes the page global directory by clearing all the page table
  entries which are not used by the kernel.
*/
void init_pgd() {
  /* Clear page table entries below the kernel. */
  for (size_t i = 0; i < (uint32_t)&VIRT_OFFSET; i += PTE_SIZE) {
    pte_clear(&memory_manager, i);
  }

  /* Clear page table entries above the kernel. */
  for (size_t i = high_memory; i < VMALLOC_BEGIN_VADDR; i += PTE_SIZE) {
    pte_clear(&memory_manager, i);
  }
}

/*
  map_smc maps the static memory controller. It allocates a page table entry
  and maps the UART from 0x1c090000 to 0xffc00000.
*/
void map_smc() {
  uint32_t* pte;

  uart_0 = (volatile struct uart_registers*)0xffc00000;
  pte = pte_alloc(&memory_manager, 0xffc00000);
  pte_insert(pte, 0xffc00000, 0x1c090000);
}

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
  offset = pgd_offset(mm->pgd, addr);
  *offset = ((uint32_t)virt_to_phys((uint32_t)pte) & 0xfffffc00) | 0x1;
  invalidate_entire_tlb();
  return pte;
}

/*
  pte_clear clears a page table entry from a virtual address "addr" in the page
  global directory specified by the memory manager "mm".
*/
void pte_clear(struct memory_manager* mm, uint32_t addr) {
  uint32_t* offset;

  offset = pgd_offset(mm->pgd, addr);
  *offset = 0x1;
  invalidate_entire_tlb();
}

/*
  pte_insert inserts a page into a page table entry "pte". The page is
  specified by which physical address "p_addr" it maps to which virtual address
  "v_addr". If a page already exists it is replaced.
*/
void pte_insert(uint32_t* pte, uint32_t v_addr, uint64_t p_addr) {
  uint32_t* offset;

  offset = pte_offset(pte, v_addr);
  *offset = ((uint32_t)p_addr & 0xfffff000) | 0x1 << 0x4 | 0x1 << 0x1;
  invalidate_entire_tlb();
}
