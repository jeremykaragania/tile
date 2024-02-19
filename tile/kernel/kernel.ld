ENTRY(_start)

SECTIONS {
    . = 0x80008000;

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
