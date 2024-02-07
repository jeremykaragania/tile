ENTRY(_start)

SECTIONS {
    PHYS_OFFSET = 0x80000000;
    VIRT_OFFSET = 0xC0000000;
    TEXT_OFFSET = 0x00008000;
    KERNEL_RAM_PADDR = PHYS_OFFSET + TEXT_OFFSET;
    KERNEL_RAM_VADDR = VIRT_OFFSET + TEXT_OFFSET;

    .text : {
        head.o(.text)
        *(.text)
    }

    .rodata : {
      *(.rodata)
    }

    .data : {
      *(.data)
    }

    .bss : {
      *(.bss)
      *(COMMON)
    }
}
