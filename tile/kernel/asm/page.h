#ifndef ASM_PAGE_H
#define ASM_PAGE_H

#define PAGE_RWX (PAGE_READ | PAGE_WRITE | PAGE_EXECUTE)
#define PAGE_RW (PAGE_READ | PAGE_WRITE)
#define PAGE_RO PAGE_READ

#define VADDR_SPACE_END 0xffffffff

#define PMD_SIZE 0x100000
#define PAGE_SIZE 0x1000
#define PAGE_TABLE_SIZE 0x400
#define PAGES_PER_PAGE_TABLE (PAGE_TABLE_SIZE / 4)

#define PG_DIR_SHIFT 0x14
#define PAGE_SHIFT 0xc
#define PAGE_MASK 0xfffff

#define PAGE_AP_0 4
#define PAGE_AP_1 5
#define PAGE_AP_2 9

#define SECTION_AP_0 10
#define SECTION_AP_1 11
#define SECTION_AP_2 15

#endif
