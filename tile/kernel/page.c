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
    pmd_clear(&memory_manager, i);
  }

  /* Clear page table entries above the kernel. */
  for (size_t i = high_memory; i < VMALLOC_BEGIN_VADDR; i += PTE_SIZE) {
    pmd_clear(&memory_manager, i);
  }
}

/*
  map_kernel maps the kernel. It sets the appropriate attributes for the kernel
  memory sections.
*/
void map_kernel() {
  /* Map the memory before the ".text" section. */
  create_mapping(phys_to_virt(KERNEL_SPACE_PADDR), KERNEL_SPACE_PADDR, memory_manager.text_begin - phys_to_virt(KERNEL_SPACE_PADDR), BLOCK_RW);
  /* Map the memory in the ".text" section. */
  create_mapping(memory_manager.text_begin, virt_to_phys(memory_manager.text_begin), memory_manager.text_end - memory_manager.text_begin, BLOCK_RWX);
  /*Map the memory after the ".text" section. */
  create_mapping(memory_manager.data_begin, virt_to_phys(memory_manager.data_begin), memory_manager.bss_end - memory_manager.data_begin, BLOCK_RW);
}

/*
  map_smc maps the static memory controller. It allocates a page middle
  directory and maps the UART from 0x1c090000 to 0xffc00000.
*/
void map_smc() {
  uint32_t* pmd;

  uart_0 = (volatile struct uart_registers*)0xffc00000;
  pmd = pmd_alloc(&memory_manager, 0xffc00000);
  pmd_insert(pmd, 0xffc00000, 0x1c090000, BLOCK_RW);
}

/*
  create_mapping creates a linear mapping from the virtual address "v_addr" to
  the physical address "p_addr" spanning "size" bytes with the memory flags
  "flags".
*/
void create_mapping(uint32_t v_addr, uint64_t p_addr, uint32_t size, int flags) {
  uint32_t* pmd;

  for (size_t i = v_addr; i < v_addr + size; i += PTE_SIZE) {
    pmd = pgd_offset(memory_manager.pgd, i);

    /* Page middle directory has many entries. */
    if (!IS_ALIGNED(i, PTE_SIZE) || i + PTE_SIZE > v_addr + size) {
      if (!pmd_is_page_table(pmd)) {
        pmd = pmd_alloc(&memory_manager, i);
      }

      for (size_t j = i; j < v_addr + size; j += PAGE_SIZE, p_addr += PAGE_SIZE) {
        pmd_insert(pmd, j, p_addr, flags);
      }
    }
    /* Page middle directory has one entry. */
    else {
      *pmd = create_pmd_section(virt_to_phys(i), flags);
    }
  }
}

/*
  pgd_offset returns the address of a page middle directory from a virtual
  address "addr", and the base address of a page global directory "pgd". In
  ARMv7-A parlance, it returns the address of a page table first-level
  descriptor from a virtual address "addr", and a translation table base,
  "pgd".
*/
uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr) {
  return (uint32_t*)((uint32_t)pgd + (uint32_t)pgd_index(addr));
}

/*
  pmd_offset returns the address of a page table entry from a virtual address
  "addr", and the base address of a page middle directory "pmd". In ARMv7-A
  parlance, it returns the address of a small page second-level descriptor from
  a virtual address, "addr" and a page table base address, "pmd".
*/
uint32_t* pmd_offset(uint32_t* pmd, uint32_t addr) {
  pmd = pmd_to_page_table(pmd);
  return (uint32_t*)((uint32_t)pmd + (uint32_t)pmd_index(addr));
}

/*
  pmd_alloc allocates a new page middle directory from a virtual address "addr"
  and returns a pointer to it. The appropriate index in the page global
  directory specified by the memory manager "mm" is updated to point to the new
  page middle directory.
*/
uint32_t* pmd_alloc(struct memory_manager* mm, uint32_t addr) {
  uint32_t* pmd;
  uint32_t* offset;

  pmd = memory_alloc(PAGE_SIZE);
  offset = pgd_offset(mm->pgd, addr);
  *offset = create_pmd_page_table(pmd);
  return offset;
}

/*
  pmd_clear clears a page middle directory from a virtual address "addr" in the
  page global directory specified by the memory manager "mm".
*/
void pmd_clear(struct memory_manager* mm, uint32_t addr) {
  uint32_t* offset;

  offset = pgd_offset(mm->pgd, addr);
  *offset = 0x1;
}

/*
  pmd_insert inserts a page table entry into a page middle directory "pmd". The
  page table entry is specified by which physical address "p_addr" it maps to
  which virtual address "v_addr" and its memory flags "flags". If a page table
  entry already exists it is replaced.
*/
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint64_t p_addr, int flags) {
  uint32_t* offset;

  offset = pmd_offset(pmd, v_addr);
  *offset = create_pte(p_addr, flags);
}

/*
  pmd_is_page_table returns a positive integer if the page middle directory
  "pmd" is a page table.
*/
int pmd_is_page_table(uint32_t* pmd) {
  return *pmd & 1;
}

/*
  pmd_to_page_table returns the page table address of a page middle directory
  "pmd" if there's one.
*/
uint32_t* pmd_to_page_table(uint32_t* pmd) {
  if (pmd_is_page_table(pmd)) {
    return (uint32_t*)phys_to_virt(*pmd & 0xfffff000);
  }

  return NULL;
}

/*
  create_pmd_section creates and returns a page middle directory entry for a
  section entry. The entry is specified by which physical address "p_addr" it
  maps to, and with which flags "flags".
*/
uint32_t create_pmd_section(uint64_t p_addr, int flags) {
  uint32_t pte = (p_addr & 0xfffffc00ull) | 1 << 1;

  switch (flags) {
    case BLOCK_RWX: {
      pte |= 1 << 10;
      break;
    }
    case BLOCK_RW:
      pte |= 1 << 4;
      pte |= 1 << 10;
      break;
    case BLOCK_RO: {
      pte |= 1 << 4;
      pte |= 1 << 10;
      pte |= 1 << 15;
      break;
    }
  }

  return pte;
}

/*
  create_pmd_page_table creates and returns a page middle directory entry for a
  page table. The entry is specified by the virtual address of the page table
  "page_table".
*/
uint32_t create_pmd_page_table(uint32_t* page_table) {
  return ((uint32_t)virt_to_phys((uint32_t)page_table) & 0xfffffc00) | 1;
}

/*
  create_pte creates and returns a page table entry for a page table. The entry
  is specified by which physical address "p_addr" it maps to, and with which
  flags "flags".
*/
uint32_t create_pte(uint64_t p_addr, int flags) {
  uint32_t pte = ((uint32_t)p_addr & 0xfffff000) | 1 << 1;

  switch (flags) {
    case BLOCK_RWX: {
      pte |= 1 << 4;
      break;
    }
    case BLOCK_RW:
      pte |= 1;
      pte |= 1 << 4;
      break;
    case BLOCK_RO: {
      pte |= 1;
      pte |= 1 << 4;
      pte |= 1 << 9;
      break;
    }
  }

  return pte;
}
