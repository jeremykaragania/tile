ENTRY(_start)

SECTIONS {
    . = 0x80000000;

    text_begin = .;
    .text : {
        asm/main.o(.vector_table)
        asm/main.o(.text)
        *(.text)
    }
    text_end = .;

    .rodata : {
      *(.rodata)
    }

    init_stack = .;
    .data.init_stack : {
      asm/main.o(.data.init_stack)
    }

    data_begin = .;
    .data : {
      *(.data)
    }
    data_end = .;

    .bss : {
      *(.bss)
      *(COMMON)
    }
}
