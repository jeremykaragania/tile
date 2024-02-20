ENTRY(_start)

SECTIONS {
    . = 0x80010000;

    .text : {
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
