#include <page.h>

/*
  pgd_offset returns the address of a page table entry from a virtual address
  "addr", and the base address of a page global directory "pgd". In ARMv7-A
  paralance, it returns the address of a first-level descriptor from a virtual
  address "addr", and a translation table base, "pgd".
*/
uint32_t* pgd_offset(uint32_t* pgd, uint32_t addr) {
  return pgd + pgd_index(addr);
}
