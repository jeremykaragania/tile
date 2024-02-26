ENTRY(_start)

SECTIONS {
    . = 0x80000000;
    .text : {
        asm/main.o(.vector_table)
        asm/main.o(.text)
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
