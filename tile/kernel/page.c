#include <page.h>

/*
  init_paging initializes the kernel's paging and maps required memory regions.
*/
void init_paging() {
  init_pgd();
  map_kernel();
  map_smc();
  invalidate_entire_tlb();
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
  map_kernel maps the kernel. It sets the appropriate attributes for the kernel
  memory sections.
*/
void map_kernel() {
  /* Map the memory before the ".text" section. */
  create_mapping(phys_to_virt(KERNEL_SPACE_PADDR), KERNEL_SPACE_PADDR, memory_manager.text_begin - phys_to_virt(KERNEL_SPACE_PADDR));
  /* Map the memory in the ".text" section. */
  create_mapping(memory_manager.text_begin, virt_to_phys(memory_manager.text_begin), memory_manager.text_end - memory_manager.text_begin);
  /*Map the memory after the ".text" section. */
  create_mapping(memory_manager.data_begin, virt_to_phys(memory_manager.data_begin), memory_manager.bss_end - memory_manager.data_begin);
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
  create_mapping creates a linear mapping from the virtual address "v_addr" to
  the physical address "p_addr" spanning "size" bytes.
*/
void create_mapping(uint32_t v_addr, uint64_t p_addr, uint32_t size) {
  uint32_t* pte;

  for (size_t i = v_addr; i < v_addr + size; i += PTE_SIZE) {
    pte = pgd_offset(memory_manager.pgd, i);

    /* Use many PAGE_SIZE entries. */
    if (!IS_ALIGNED(i, PTE_SIZE) || i + PTE_SIZE > v_addr + size) {
      if (!pte_is_page_table(pte)) {
        pte = pte_alloc(&memory_manager, i);
      }
      else {
        pte = pte_to_page_table(pte);
      }

      for (size_t j = i; j < v_addr + size; j += PAGE_SIZE, p_addr += PAGE_SIZE) {
        pte_insert(pte, j, p_addr);
      }
    }
    /* Use one PTE_SIZE entry. */
    else {
      *pte = i | 0x402;
    }
  }
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
}

/*
  pte_is_page_table returns a positive integer if the page table entry "pte" is
  a page table.
*/
int pte_is_page_table(uint32_t* pte) {
  return *pte & 1;
}

/*
  pte_to_page_table returns the page table address of a page table entry "pte"
  if there's one.
*/
uint32_t* pte_to_page_table(uint32_t* pte) {
  if (pte_is_page_table(pte)) {
    return (uint32_t*)phys_to_virt(*pte & 0xfffff000);
  }
  return NULL;
}
