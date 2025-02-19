#ifndef ASM_MEMORY_H
#define ASM_MEMORY_H

#define PHYS_OFFSET 0x80000000
#define VIRT_OFFSET 0xc0000000
#define TEXT_OFFSET 0x00008000
#define THREAD_SIZE 0x00002000

#define KERNEL_SPACE_PADDR 0x80000000ull

#define VECTOR_TABLE_VADDR 0xffff0000

#define UART_0_PADDR 0x1c090000ull
#define UART_0_VADDR 0xffc00000

#define SMC_CS3_PADDR 0x1c000000ull
#define MCI_PADDR (SMC_CS3_PADDR + 0x50000)
#define MCI_VADDR 0xffc01000

#define GIC_PADDR 0x2c000000ull
#define GICD_PADDR (GIC_PADDR + 0x1000)
#define GICD_VADDR 0xffc02000
#define GICC_PADDR (GIC_PADDR + 0x2000)
#define GICC_VADDR 0xffc03000

#define PG_DIR_SIZE 0x00004000ull
#define PG_DIR_PADDR (KERNEL_SPACE_PADDR + PG_DIR_SIZE)

#define VMALLOC_OFFSET 0x800000
#define VMALLOC_MIN_VADDR 0xf0000000
#define VMALLOC_BEGIN_VADDR high_memory + VMALLOC_OFFSET
#define VMALLOC_END_VADDR 0xff800000

#endif
