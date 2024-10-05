#include <kernel/page.h>

static uint32_t virt_bitmap_data[VIRT_BITMAP_SIZE];

struct memory_bitmap virt_bitmap = {
  virt_bitmap_data,
  VIRT_BITMAP_SIZE,
  0,
  NULL
};

/*
  init_paging initializes the kernel's paging and maps required memory regions.
  Currently the kernel is mapped using 1MB sections. These sections are
  replaced with 4KB small pages where required.
*/
void init_paging() {
  init_pgd();
  map_kernel();
  map_vector_table();
  map_smc();
  invalidate_entire_tlb();
}

/*
  init_pgd initializes the page global directory by clearing all the page table
  entries which are not used by the kernel.
*/
void init_pgd() {
  /* Reserve all the pages in the virtual bitmap. */
  for (size_t i = 0; i < virt_bitmap.size; ++i) {
    virt_bitmap.data[i] = 0xffffffff;
  }

  /* Clear page table entries below the kernel. */
  for (uint32_t i = 0; i < (uint32_t)&VIRT_OFFSET; i += PMD_SIZE) {
    pmd_clear(memory_manager.pgd, i);
  }

  /* Clear page table entries above the kernel. */
  for (uint32_t i = ALIGN(memory_manager.bss_end, PMD_SIZE); i < VMALLOC_BEGIN_VADDR; i += PMD_SIZE) {
    pmd_clear(memory_manager.pgd, i);
  }

  /* Clear virtual bitmap entries above the kernel. */
  for (uint32_t i = memory_manager.bss_end; i < VMALLOC_BEGIN_VADDR; i += PAGE_SIZE) {
    bitmap_clear(&virt_bitmap, i);
  }
}

/*
  map_kernel maps the kernel. It sets the appropriate attributes for the kernel
  memory sections.
*/
void map_kernel() {
  /*Map the memory after the ".text" section. */
  create_mapping(memory_manager.data_begin, virt_to_phys(memory_manager.data_begin), memory_manager.bss_end - memory_manager.data_begin, BLOCK_RW);
  /* Map the memory before the ".text" section. */
  create_mapping(phys_to_virt(KERNEL_SPACE_PADDR), KERNEL_SPACE_PADDR, memory_manager.text_begin - phys_to_virt(KERNEL_SPACE_PADDR), BLOCK_RW);
  /* Map the memory in the ".text" section. */
  create_mapping(memory_manager.text_begin, virt_to_phys(memory_manager.text_begin), memory_manager.text_end - memory_manager.text_begin, BLOCK_RWX);

  /* Map the reserved memory blocks after the ".bss" section. */
  for (size_t i = 0; i < memory_map.reserved->size; ++i) {
    if (phys_to_virt(memory_map.reserved->blocks[i].begin) >= memory_manager.bss_end) {
      create_mapping(phys_to_virt(memory_map.reserved->blocks[i].begin), memory_map.reserved->blocks[i].begin, memory_map.reserved->blocks[i].size, BLOCK_RW);
    }
  }
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
  uart_0 = (volatile struct uart_registers*)0xffc00000;
  create_mapping((uint32_t)mci, (uint32_t)mci, PAGE_SIZE, BLOCK_RW);
  create_mapping((uint32_t)uart_0, 0x1c090000, PAGE_SIZE, BLOCK_RW);
}

/*
  virt_page_alloc allocates a virtual page in kernel space with the flags
  "flags" and returns a pointer to it.
*/
uint32_t* virt_page_alloc(int flags) {
  uint32_t* p_addr = NULL;
  uint32_t* v_addr = NULL;

  v_addr = bitmap_alloc(&virt_bitmap);

  if (!v_addr) {
    return NULL;
  }

  p_addr = (uint32_t*)virt_to_phys((uint32_t)v_addr);
  bitmap_insert(&phys_bitmaps, (uint32_t)p_addr, PAGE_SIZE);
  create_mapping((uint32_t)v_addr, (uint32_t)p_addr, PAGE_SIZE, flags);
  return v_addr;
}

/*
  virt_page_free unmaps the virtual page which "addr" points to.
*/
int virt_page_free(uint32_t* addr) {
  bitmap_clear(&virt_bitmap, (uint32_t)addr);
  pte_clear(pgd_offset(memory_manager.pgd, (uint32_t)addr), (uint32_t)addr);
  return 1;
}

/*
  create_mapping creates a linear mapping from the virtual address "v_addr" to
  the physical address "p_addr" spanning "size" bytes with the memory flags
  "flags".
*/
void create_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags) {
  uint32_t* pmd;

  for (uint32_t i = v_addr, j = p_addr; i < v_addr + size; i += PMD_SIZE, j += PMD_SIZE) {
    pmd = pgd_offset(memory_manager.pgd, i);

    /* Page middle directory has many entries. */
    if (!IS_ALIGNED(size, PMD_SIZE)) {
      if (!pmd_is_page_table(pmd)) {
        pmd = pmd_alloc(memory_manager.pgd, i);
      }

      for (uint32_t k = i; k < v_addr + size; k += PAGE_SIZE, p_addr += PAGE_SIZE) {
        pmd_insert(pmd, k, p_addr, flags);
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

  bitmap_insert(&virt_bitmap, v_addr, size);

}

/*
  addr_is_mapped checks if the the virtual address "addr" is mapped to a
  physical address.
*/
int addr_is_mapped(uint32_t* addr) {
  return (virt_bitmap.data[bitmap_index(&virt_bitmap, (uint32_t)addr)] & 1 << bitmap_index_index(&virt_bitmap, (uint32_t)addr)) != 0;
}

/*
  pgd_offset returns the address of a page middle directory from a virtual
  address "addr", and the base address of a page global directory "pgd". It
  returns the address of a page table first-level descriptor from a virtual
  address "addr", and a translation table base, "pgd".
*/
uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr) {
  return (uint32_t*)((uint32_t)pgd + pgd_index(addr));
}

/*
  pmd_offset returns the address of a page table entry from a virtual address
  "addr", and the base address of a page middle directory "pmd". It returns the
  address of a small page second-level descriptor from a virtual address,
  "addr" and a page table base address, "pmd".
*/
uint32_t* pmd_offset(uint32_t* pmd, uint32_t addr) {
  pmd = pmd_to_page_table(pmd);
  return (uint32_t*)((uint32_t)pmd + pmd_index(addr));
}

/*
  pmd_alloc allocates a new page middle directory from a virtual address "addr"
  and returns a pointer to it. The appropriate index in the page global
  directory specified by the page global directory "pgd" is updated to point to
  the new page middle directory.
*/
uint32_t* pmd_alloc(uint32_t* pgd, uint32_t addr) {
  uint32_t* pmd;
  uint32_t* offset;

  pmd = memory_map_alloc(PAGE_TABLE_SIZE);

  /* Sometimes the pointer to the newly allocated memory isn't mapped. */
  if (!addr_is_mapped(pmd)) {
    create_mapping((uint32_t)pmd, virt_to_phys((uint32_t)pmd), PAGE_SIZE, BLOCK_RWX);
  }

  offset = pgd_offset(pgd, addr);
  *offset = create_pmd_page_table(pmd);
  return offset;
}

/*
  pmd_clear clears a page middle directory from a virtual address "addr" in the
  page global directory "pgd"
*/
void pmd_clear(uint32_t* pgd, uint32_t addr) {
  uint32_t* offset;

  offset = pgd_offset(memory_manager.pgd, addr);
  *offset = 0x0;
}

/*
  pte_clear clears a page table entry from a virtual address "addr" in the page
  middle directory "pmd".
*/
void pte_clear(uint32_t* pmd, uint32_t addr) {
  uint32_t* offset;

  offset = pmd_offset(pmd, addr);
  *offset = 0x0;
}

/*
  pmd_insert inserts a page table entry into a page middle directory "pmd". The
  page table entry is specified by which physical address "p_addr" it maps to
  which virtual address "v_addr" and its memory flags "flags". If a page table
  entry already exists it is replaced.
*/
void pmd_insert(uint32_t* pmd, uint32_t v_addr, uint32_t p_addr, int flags) {
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
  pmd_section_to_addr returns the physical address which the page middle
  directory section "pmd" maps to.
*/
uint32_t pmd_section_to_addr(uint32_t pmd) {
  return pmd & 0xfff00000;
}

/*
  pte_to_addr returns the physical address which the page table entry "pte"
  maps to.
*/
uint32_t pte_to_addr(uint32_t pte) {
  return pte & 0xfffff000;
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
