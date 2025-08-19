#ifndef ASM_PAGE_H
#define ASM_PAGE_H

#define VADDR_SPACE_END 0xffffffff

#define PMD_SIZE 0x100000
#define PAGE_SIZE 0x1000
#define PAGE_TABLE_SIZE 0x400
#define PAGES_PER_PAGE_TABLE (PAGE_TABLE_SIZE / 4)

#define PG_DIR_SHIFT 0x14
#define PAGE_SHIFT 0xc
#define PAGE_MASK 0xfffff

#endif
