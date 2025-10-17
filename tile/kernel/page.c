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

#include <kernel/page.h>
#include <drivers/gic_400.h>
#include <drivers/pl011.h>
#include <drivers/pl180.h>
#include <drivers/sp804.h>
#include <kernel/memory.h>
#include <kernel/process.h>
#include <lib/string.h>
#include <limits.h>

const struct descriptor_bits pmd_section_bits = {
  .ap = {10, 11, 15},
  .xn = 4
};

const struct descriptor_bits pte_bits = {
  .ap = {4, 5, 9},
  .xn = 0
};

/*
  init_paging initializes the kernel's paging and maps required memory regions.
  Currently the kernel is mapped using 1MB sections. These sections are
  replaced with 4KB small pages where required.
*/
void init_paging() {
  list_init(&init_process.mem->pages_head);
  create_page_region_bounds(&init_process.mem->pages_head);
  init_pgd();
  map_kernel();
  map_peripherals();
  map_smc();
  map_vector_table();
  invalidate_entire_tlb();
}

/*
  init_pgd initializes the page global directory by clearing all the page table
  entries which are not used by the kernel.
*/
void init_pgd() {
  uint32_t* pgd = current->mem->pgd;

  /* Clear page table entries below the kernel. */
  for (uint32_t i = 0; i < VIRT_OFFSET; i += PMD_SIZE) {
    pmd_clear(pgd, i);
  }

  /* Clear page table entries above the kernel. */
  for (uint32_t i = high_memory; i < VMALLOC_BEGIN_VADDR; i += PMD_SIZE) {
    pmd_clear(pgd, i);
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
  struct memory_info* mem = init_process.mem;
  uint32_t text_begin = pmd_to_addr(mem->pgd, addr_to_pmd(mem->pgd, mem->text_begin));
  uint32_t text_end = pmd_to_addr(mem->pgd, addr_to_pmd(mem->pgd, mem->text_end)) + PMD_SIZE;

  /* Map the memory in the ".text" section. */
  create_mapping(mem->text_begin, virt_to_phys(mem->text_begin), mem->text_end - mem->text_begin, PAGE_RWX | PAGE_KERNEL);

  /* Map the section memory before the ".text" section. */
  create_mapping(text_begin, virt_to_phys(text_begin), mem->text_begin - text_begin, PAGE_RW | PAGE_KERNEL);

  /* Map the section memory after the ".text" section. */
  create_mapping(ALIGN(mem->text_end, PAGE_SIZE), virt_to_phys(ALIGN(mem->text_end, PAGE_SIZE)), text_end - mem->text_end, PAGE_RW | PAGE_KERNEL);

  /* Map the kernel memory after the ".text" section. */
  create_mapping(text_end + PMD_SIZE, virt_to_phys(text_end + PMD_SIZE), ALIGN(high_memory - (text_end + PMD_SIZE), PMD_SIZE), PAGE_RW | PAGE_KERNEL);
}

/*
  map_peripherals maps the peripherals. It maps the global interrupt
  controller.
*/
void map_peripherals() {
  create_mapping(GICD_VADDR, (uint32_t)gicd, PAGE_SIZE, PAGE_RW | PAGE_KERNEL);
  create_mapping(GICC_VADDR, (uint32_t)gicc, PAGE_SIZE, PAGE_RW | PAGE_KERNEL);
  gicd = (volatile struct gic_distributor_registers*)GICD_VADDR;
  gicc = (volatile struct gic_cpu_interface_registers*)GICC_VADDR;
}

/*
  map_vector_table maps the interrupt vector table. All it really does is make
  the memory executable.
*/
void map_vector_table() {
  create_mapping(VECTOR_TABLE_VADDR, virt_to_phys((uint32_t)&vector_table_begin), &interrupts_end - &vector_table_begin, PAGE_RWX | PAGE_KERNEL);
}

/*
  map_smc maps the static memory controller. It allocates a page middle
  directory and maps the MCI and the UART.
*/
void map_smc() {
  mci = create_mapping(MCI_VADDR, (uint32_t)mci, PAGE_SIZE, PAGE_RW | PAGE_KERNEL);
  uart_0 = create_mapping(UART_0_VADDR, UART_0_PADDR, PAGE_SIZE, PAGE_RW | PAGE_KERNEL);
  timer_0 = create_mapping(TIMER_1_VADDR, (uint32_t)timer_0, PAGE_SIZE, PAGE_RW | PAGE_KERNEL);
}

/*
  create_mapping creates a linear mapping from the virtual address "v_addr" to
  the physical address "p_addr" spanning "size" bytes with the memory flags
  "flags". It returns a pointer to the mapped area.
*/
void* create_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags) {
  const uint32_t ret = v_addr;
  const uint32_t count = page_count(size);
  const uint32_t begin = v_addr;
  const uint32_t end = v_addr + size;
  size_t i = 0;
  uint32_t step;

  size = ALIGN(size, PAGE_SIZE);

  while (i < count) {
    if (IS_ALIGNED(v_addr, PMD_SIZE) && v_addr + PMD_SIZE < end) {
      step = PMD_SIZE;
      create_section_mapping(v_addr, p_addr, step, flags);
    }
    else {
      step = PAGE_SIZE;
      create_page_mapping(v_addr, p_addr, step, flags);
    }

    v_addr += step;
    p_addr += step;
    i += page_count(step);
  }

  create_page_region(&current->mem->pages_head, begin, count, flags);

  return (void*)ret;
}

/*
  create_section_mapping creates a linear mapping from the virtual address
  "v_addr" to the physical address "p_addr" spanning "size" bytes with the
  memory flags "flags" and returns a pointer to the mapped area. It uses
  sections for the mapping. This function shouldn't be called directly. Use
  create_mapping instead.
*/
void* create_section_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags) {
  uint32_t* pgd = current->mem->pgd;
  uint32_t* pmd;

  for (size_t i = 0; i < pmd_count(size); ++i, v_addr += PMD_SIZE, p_addr += PMD_SIZE) {
    pmd = addr_to_pmd(pgd, v_addr);
    *pmd = create_pmd_section(p_addr, flags);
  }

  return (void*)v_addr;
}

/*
  create_section_mapping creates a linear mapping from the virtual address
  "v_addr" to the physical address "p_addr" spanning "size" bytes with the
  memory flags "flags" and returns a pointer to the mapped area. It uses pages
  for the mapping. This function shouldn't be called directly. Use
  create_mapping instead.
*/
void* create_page_mapping(uint32_t v_addr, uint32_t p_addr, uint32_t size, int flags) {
  uint32_t* pgd = current->mem->pgd;
  uint32_t* pmd;
  uint32_t pmd_begin;
  uint32_t pmd_end;
  uint32_t* page_table;
  uint32_t* insert_pmd;
  int is_page_table;
  uint32_t pmd_page_table;
  uint32_t end = v_addr + size;

  for (size_t i = 0; i < pmd_count(size); ++i) {
    pmd = addr_to_pmd(pgd, v_addr);
    pmd_begin = pmd_to_addr(pgd, pmd);
    pmd_end = pmd_begin + PMD_SIZE - 1;
    is_page_table = pmd_is_page_table(pmd);

    /*
      This pmd isn't a page table, so we will allocate and replace it with
      a new one.
    */
    if (!is_page_table) {
      page_table = memory_alloc(PAGE_TABLE_SIZE);
      pmd_page_table = create_pmd_page_table(page_table);
      insert_pmd = &pmd_page_table;
    }
    else {
      insert_pmd = pmd;
    }

    /*
      If the new page table should map itself, then we make sure that it
      does.
    */
    if ((uint32_t)page_table < pmd_end && (uint32_t)page_table > pmd_begin) {
      pmd_insert(insert_pmd, (uint32_t)page_table, virt_to_phys((uint32_t)page_table), PAGE_RW | PAGE_KERNEL);
    }

    /* We either insert into the new page table or the existing one. */
    while (v_addr < pmd_end && v_addr < end) {
      pmd_insert(insert_pmd, v_addr, p_addr, flags);

      v_addr += PAGE_SIZE;
      p_addr += PAGE_SIZE;
    }

    /*
      This is last as we have to populate the new page table before replacing
      the pmd.
    */
    if (!is_page_table) {
      *pmd = pmd_page_table;
    }
  }

  return (void*)v_addr;
}

/*
  find_unmapped_region finds a region of size "size" in the current process's
  virtual memory. On success, It returns a pointer to the region.
*/
void* find_unmapped_region(uint32_t size) {
  const struct list_link* pages_head = &(current->mem->pages_head);
  struct list_link* curr = pages_head->next;
  struct page_region* curr_data;
  struct page_region* next_data;
  uint32_t gap;

  size = ALIGN(size, PAGE_SIZE);

  while (curr->next != pages_head) {
    curr_data = list_data(curr, struct page_region, link);
    next_data = list_data(curr->next, struct page_region, link);
    gap = next_data->begin - page_region_end(curr_data);

    if (gap >= size) {
      return (void*)page_region_end(curr_data);
    }

    curr = curr->next;
  }

  return NULL;
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
  return (((uint32_t)pmd - (uint32_t)pgd) >> 2) * PMD_SIZE;
}

/*
  pmd_section_to_addr returns the physical address which the page middle
  directory section "pmd" maps to.
*/
uint32_t pmd_section_to_addr(uint32_t pmd) {
  return pmd & 0xfff00000;
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

  pmd = addr_to_pmd(current->mem->pgd, addr);
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
  create_pgd creates a new page global directory and copies the kernel mappings
  into it since they must remain present in all processes.
*/
void* create_pgd() {
  uint32_t* pgd;
  uint32_t* init_pgd;
  uint32_t pgd_virt_offset;

  init_pgd = current->mem->pgd;
  pgd = memory_alloc(PG_DIR_SIZE);
  pgd_virt_offset = (uint32_t)addr_to_pmd(pgd, VIRT_OFFSET) - (uint32_t)pgd;

  /* Zero out the userspace entries in the new PGD. */
  memset(pgd, 0, pgd_virt_offset);

  /* Copy the kernelspace entries from the initial PGD into the new PGD. */
  memcpy((void*)((uint32_t)pgd + pgd_virt_offset), (void*)((uint32_t)init_pgd + pgd_virt_offset), PG_DIR_SIZE - pgd_virt_offset);

  return pgd;
}

/*
  create_pmd_section creates and returns a page middle directory entry for a
  section entry. The entry is specified by which physical address "p_addr" it
  maps to, and with which flags "flags".
*/
uint32_t create_pmd_section(uint32_t p_addr, int flags) {
  uint32_t pmd = (p_addr & 0xfffffc00) | 1 << 1;

  pmd = set_descriptor_protection(pmd, &pmd_section_bits, flags);

  return pmd;
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
  flags "flags". If a page is executable or writable, then it is at least
  readable.
*/
uint32_t create_pte(uint32_t p_addr, int flags) {
  uint32_t pte = (p_addr & 0xfffff000) | 1 << 1;

  pte = set_descriptor_protection(pte, &pte_bits, flags);

  return pte;
}

/*
  set_descriptor_protection sets the memory protection of the descriptor
  "descriptor". The descriptor bit indexes are specified by "bits" and the
  memory protection by "flags".
*/
uint32_t set_descriptor_protection(uint32_t descriptor, const struct descriptor_bits* bits, int flags) {
  if (!flags) {
    return descriptor;
  }

  if (flags & PAGE_KERNEL) {
    descriptor |= 1 << bits->ap[0];
  }
  else {
    descriptor |= 1 << bits->ap[0];
    descriptor |= 1 << bits->ap[1];
  }

  if (!(flags & PAGE_WRITE)) {
    descriptor |= 1 << bits->ap[2];
  }

  if (!(flags & PAGE_EXECUTE)) {
    descriptor |= 1 << bits->xn;
  }

  return descriptor;
}

/*
  create_page_region_bounds initializes the bounds of a page region list with
  head "head". It creates dummy lower and upper bound page regions in the
  virtual address space.
*/
void create_page_region_bounds(struct list_link* head) {
  create_page_region(head, 0, 1, 0);
  create_page_region(head, VADDR_SPACE_END, 0, 0);
}

/*
  create_page region allocates a page region and inserts it into the page
  region list.
*/
struct page_region* create_page_region(struct list_link* head, uint32_t begin, size_t count, int flags) {
  struct page_region* region = memory_alloc(sizeof(struct page_region));

  if (!region) {
    return NULL;
  }

  region->begin = begin;
  region->count = count;
  region->flags = flags;
  insert_page_region(head, region);

  return region;
}


/*
  insert_page_region inserts a page region in the page region list with head
  "head" while preserving a page region address ordering.
*/
void insert_page_region(struct list_link* head, struct page_region* region) {
  struct list_link* curr = head->next;
  uint32_t region_end = page_region_end(region);
  struct page_region* curr_region;
  uint32_t curr_end;

  while(curr != head) {
    curr_region = list_data(curr, struct page_region, link);
    curr_end = page_region_end(curr_region);

    if (region_end <= curr_region->begin) {
      break;
    }

    if (region->begin >= curr_end) {
      curr = curr->next;
      continue;
    }

    if (region->begin > curr_region->begin && region->begin < curr_end) {
      curr_region = split_page_region(curr_region, page_index(region->begin) - page_index(curr_region->begin));
      curr = &curr_region->link;
    }

    if (region_end <= curr_end && region_end > curr_region->begin) {
      split_page_region(curr_region, page_index(region_end) - page_index(curr_region->begin));
    }

    list_remove(head, curr);
    curr = curr->next;
  }

  list_push(curr->prev, &region->link);
}

/*
  remove_page_region removes the page region "region" from the page region list
  with head "head".
*/
void remove_page_region(struct list_link* head, struct page_region* region) {
  list_remove(head, &region->link);
  memory_free(region);
}

/*
  split_page_region splits the page region "region" at the page index "index"
  into two page regions, a left one and a right one. The left one is just the
  original region resized and the right one is allocated. On success, the right
  page region is returned.
*/
struct page_region* split_page_region(struct page_region* region, size_t index) {
  const size_t region_count = region->count;
  struct page_region* insert_region;

  if (index >= region->count) {
    return NULL;
  }

  region->count = index;

  insert_region = memory_alloc(sizeof(struct page_region));

  if (!insert_region) {
    return NULL;
  }

  insert_region->begin = page_region_end(region);
  insert_region->count = region_count - index;

  list_push(&region->link, &insert_region->link);

  return insert_region;
}

/*
  find_page region returns the page region which the virtual address "addr" is
  inside. If "addr" is inside no page region, then NULL is returned.
*/
struct page_region* find_page_region(struct list_link* head, uint32_t addr) {
  struct list_link* pages_head = &current->mem->pages_head;
  struct list_link* curr = pages_head->next;
  struct page_region* region;
  uint32_t region_end;

  while (curr != pages_head) {
    region = list_data(curr, struct page_region, link);
    region_end = page_region_end(region);

    if (addr >= region->begin && addr < region_end) {
      return region;
    }

    curr = curr->next;
  }

  return NULL;
}
