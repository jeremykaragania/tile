PHYS_OFFSET = 0x80000000;
TEXT_OFFSET = 0x00008000;
THREAD_SIZE = 0x00002000;
PAGE_SIZE = 0x00001000;

ENTRY(_start)

SECTIONS {
    . = PHYS_OFFSET;
    . += TEXT_OFFSET;

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

    data_begin = .;
    .data : {
      . = ALIGN(PAGE_SIZE);
      init_begin = .;
      main.o(.init_process_table_entry)
      . = init_begin + THREAD_SIZE;
      init_end = .;
      *(.data)
    }
    data_end = .;

    .bss : {
      *(.bss)
      *(COMMON)
    }
}
