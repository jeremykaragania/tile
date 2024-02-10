ENTRY(_start)

SECTIONS {
    . = 0x80008000;

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
