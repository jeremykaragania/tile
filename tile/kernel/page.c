#include <kernel/page.h>

/*
  page.c handles virtual memory.

  When translating a virtual address into a physical address there can be up to
  two levels of translation. This mirrors the ARMv7-A virtual Memory System
  Architecture (VMSA) in semantics. However, the virtual memory terminology
  used is similar to that of Linux.

  A page global directory (PGD) is the first level of translation. Each of its
  entries maps 1MB of virtual memory. It can either map to a page middle
  directory (PMD) a section. If it maps to a section, the translation stops
  here, otherwise it continues to a PMD.

  A PMD is the optional second level of translation. Each of its entries maps
  page (4KB) of virtual memory. It maps to a page table entry (PTE).
  Translation stops here.
*/

/*
  init_paging initializes the kernel's paging and maps required memory regions.
  Currently the kernel is mapped using 1MB sections. These sections are
  replaced with 4KB small pages where required.
*/
void init_paging() {
  init_pgd();
  map_kernel();
  map_smc();
  map_vector_table();
  invalidate_entire_tlb();
}

/*
  init_pgd initializes the page global directory by clearing all the page table
  entries which are not used by the kernel.
*/
void init_pgd() {
  /* Clear page table entries below the kernel. */
  for (uint32_t i = 0; i < VIRT_OFFSET; i += PMD_SIZE) {
    pmd_clear(memory_manager.pgd, i);
  }

  /* Clear page table entries above the kernel. */
  for (uint32_t i = high_memory; i < VMALLOC_BEGIN_VADDR; i += PMD_SIZE) {
    pmd_clear(memory_manager.pgd, i);
  }
}

/*
  map_kernel maps the kernel. It sets the appropriate attributes for the kernel
  memory sections. The ".text" section is the only section which we map at a
  page granularity. All other memory uses a page middle directory section
  granularity. "text_begin" and "text_end" are respectively the page middle
  directory lower and upper section bounds of the ".text" section. We need to
  make sure that the pages below and above the ".text" section are mapped too.
*/
void map_kernel() {
  uint32_t text_begin = pmd_to_addr(memory_manager.pgd, addr_to_pmd(memory_manager.pgd, memory_manager.text_begin));
  uint32_t text_end = pmd_to_addr(memory_manager.pgd, addr_to_pmd(memory_manager.pgd, memory_manager.text_end)) + PMD_SIZE;

  /* Map the memory in the ".text" section. */
  create_mapping(memory_manager.text_begin, virt_to_phys(memory_manager.text_begin), memory_manager.text_end - memory_manager.text_begin, BLOCK_RWX);

  /* Map the section memory before the ".text" section. */
  create_mapping(text_begin, virt_to_phys(text_begin), memory_manager.text_begin - text_begin, BLOCK_RW);

  /* Map the section memory after the ".text" section. */
  create_mapping(ALIGN(memory_manager.text_end, PAGE_SIZE), virt_to_phys(ALIGN(memory_manager.text_end, PAGE_SIZE)), text_end - memory_manager.text_end, BLOCK_RW);

  /* Map the kernel memory before the ".text" section. */
  create_mapping(VIRT_OFFSET, virt_to_phys(VIRT_OFFSET), ALIGN(text_begin - VIRT_OFFSET, PMD_SIZE), BLOCK_RW);

  /* Map the kernel memory after the ".text" section. */
  create_mapping(text_end + PMD_SIZE, virt_to_phys(text_end + PMD_SIZE), ALIGN(high_memory - (text_end + PMD_SIZE), PMD_SIZE), BLOCK_RW);
}

/*
  map_vector_table maps the interrupt vector table. All it really does is make
  the memory executable.
*/
void map_vector_table() {
  create_mapping(0xffff0000, virt_to_phys((uint32_t)&vector_table_begin), &interrupts_end - &vector_table_begin, BLOCK_RWX);
}

/*
  map_smc maps the static memory controller. It allocates a page middle
  directory and maps the MCI and the UART.
*/
void map_smc() {
  uint32_t uart_0_addr = 0xffc00000;

  create_mapping((uint32_t)mci, (uint32_t)mci, PAGE_SIZE, BLOCK_RW);
  create_mapping(uart_0_addr, 0x1c090000, PAGE_SIZE, BLOCK_RW);
  uart_0 = (volatile struct uart_registers*)uart_0_addr;
}

/*
  create_mapping creates a linear mapping from the virtual address "v_addr" to
  the physical address "p_addr" spanning "size" bytes with the memory flags
  "flags".
*/
void create_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags) {
  uint32_t* pmd;
  uint32_t pmd_addr;
  uint32_t* insert_pmd;
  uint32_t* page_table;
  uint32_t pmd_page_table;

  for (uint32_t i = v_addr, j = p_addr; i < v_addr + size; i += PMD_SIZE, j += PMD_SIZE) {
    pmd = addr_to_pmd(memory_manager.pgd, i);
    pmd_addr = pmd_to_addr(memory_manager.pgd, pmd);

    /* Page middle directory has many entries. */
    if (!IS_ALIGNED(size, PMD_SIZE)) {
      int is_page_table = pmd_is_page_table(pmd);

      insert_pmd = pmd;

      /*
        This pmd isn't a page table, so we will allocate and replace it with
        a new one.
      */
      if (!is_page_table) {
        page_table = memory_alloc(PAGE_TABLE_SIZE);
        pmd_page_table = create_pmd_page_table(page_table);
        insert_pmd = &pmd_page_table;

        /*
          If the new page table should map itself, then we make sure that it
          does.
        */
        if ((uint32_t)page_table > pmd_addr && (uint32_t)page_table < pmd_addr + PMD_SIZE) {
          pmd_insert(insert_pmd, (uint32_t)page_table, virt_to_phys((uint32_t)page_table), BLOCK_RW);
        }
      }

      /* We either insert into the new page table or the existing one. */
      for (uint32_t k = i; k < v_addr + size; k += PAGE_SIZE, p_addr += PAGE_SIZE) {
        pmd_insert(insert_pmd, k, p_addr, flags);
      }

      /* We have to populate the new page table before replacing the pmd. */
      if (!is_page_table) {
        *pmd = create_pmd_page_table(page_table);
      }
    }
    /* Page middle directory has one entry. */
    else {
      *pmd = create_pmd_section(j, flags);
    }

    if (i > UINT_MAX - PMD_SIZE) {
      break;
    }
  }

  bitmap_insert(&virt_bitmap, v_addr, size / PAGE_SIZE);

}

/*
  addr_to_pmd returns the address of a page middle directory from a virtual
  address "addr", and the base address of a page global directory "pgd". It
  returns the address of a page table first-level descriptor from a virtual
  address "addr", and a translation table base, "pgd".
*/
uint32_t* addr_to_pmd(uint32_t* pgd, uint32_t addr) {
  return (uint32_t*)((uint32_t)pgd + pgd_index(addr));
}

/*
  addr_to_pte returns the address of a page table entry from a virtual address
  "addr", and the base address of a page middle directory "pmd". It returns the
  address of a small page second-level descriptor from a virtual address,
  "addr" and a page table base address, "pmd".
*/
uint32_t* addr_to_pte(uint32_t* pmd, uint32_t addr) {
  pmd = pmd_to_page_table(pmd);
  return (uint32_t*)((uint32_t)pmd + pmd_index(addr));
}

/*
  pmd_to_addr returns the virtual address which the page middle directory "pmd"
  maps to in the page global directory "pgd".
*/
uint32_t pmd_to_addr(uint32_t* pgd, uint32_t* pmd) {
  return ((uint32_t)pmd - (uint32_t)pgd) / 4 * PMD_SIZE;
}

/*
  pte_to_addr returns the virtual address which the page table entry "pte" maps
  to.
*/
uint32_t pte_to_addr(uint32_t pte) {
  return pte & 0xfffff000;
}

/*
  pmd_clear clears a page middle directory from a virtual address "addr" in the
  page global directory "pgd"
*/
void pmd_clear(uint32_t* pgd, uint32_t addr) {
  uint32_t* pmd;

  pmd = addr_to_pmd(memory_manager.pgd, addr);
  *pmd = 0x0;
}

/*
  pte_clear clears a page table entry from a virtual address "addr" in the page
  middle directory "pmd".
*/
void pte_clear(uint32_t* pmd, uint32_t addr) {
  uint32_t* pte;

  pte = addr_to_pte(pmd, addr);
  *pte = 0x0;
}

/*
  pmd_insert inserts a page table entry into a page middle directory "pmd". The
  page table entry is specified by which physical address "p_addr" it maps to
  which virtual address "v_addr" and its memory flags "flags". If a page table
  entry already exists it is replaced.
*/
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint32_t p_addr, int flags) {
  uint32_t* pte;

  pte = addr_to_pte(pmd, v_addr);
  *pte = create_pte(p_addr, flags);
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
    return (uint32_t*)phys_to_virt(*pmd & 0xfffffc00);
  }

  return NULL;
}

/*
  create_pmd_section creates and returns a page middle directory entry for a
  section entry. The entry is specified by which physical address "p_addr" it
  maps to, and with which flags "flags".
*/
uint32_t create_pmd_section(uint32_t p_addr, int flags) {
  uint32_t pte = (p_addr & 0xfffffc00) | 1 << 1;

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
uint32_t create_pte(uint32_t p_addr, int flags) {
  uint32_t pte = (p_addr & 0xfffff000) | 1 << 1;

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
