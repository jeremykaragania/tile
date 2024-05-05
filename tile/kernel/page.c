#include <page.h>

/*
  pgd_offset returns the address of a page table entry from a virtual address
  "addr", and the base address of a page global directory "pgd". In ARMv7-A
  parlance, it returns the address of a page table first-level descriptor from
  a virtual address "addr", and a translation table base, "pgd".
*/
uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr) {
  return pgd + pgd_index(addr);
}

/*
  pte_offset returns the address of a page from a virtual address "addr", and
  the base address of a page table entry "pte". In ARMv7-A parlance, it returns
  the address of a small page second-level descriptor from a virtual address,
  "addr" and a page table base address, "pte".
*/
uint32_t* pte_offset(uint32_t* pte, uint32_t addr) {
  return pte + pte_index(addr);
}
