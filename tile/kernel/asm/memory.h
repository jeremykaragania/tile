#ifndef ASM_MEMORY_H
#define ASM_MEMORY_H

#define KERNEL_SPACE_PADDR 0x80000000ull

#define SMC_PADDR 0x1c000000ull

#define PG_DIR_SIZE 0x00004000ull
#define PG_DIR_PADDR (KERNEL_SPACE_PADDR + PG_DIR_SIZE)

#define VMALLOC_OFFSET 0x800000
#define VMALLOC_MIN_VADDR 0xf0000000
#define VMALLOC_BEGIN_VADDR high_memory + VMALLOC_OFFSET
#define VMALLOC_END_VADDR 0xff800000

#endif
