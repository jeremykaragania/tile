ENTRY(_start)

SECTIONS {
    . = 0x0;

    .text : {
        main.o(.vector_table)
        main.o(.text)
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
