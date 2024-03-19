PHYS_OFFSET = 0x80000000;
VIRT_OFFSET = 0xc0000000;
TEXT_OFFSET = 0x00008000;
THREAD_SIZE = 0x00002000;
PAGE_SIZE = 0x00001000;

ENTRY(_start)

SECTIONS {
    . = PHYS_OFFSET;
    . += TEXT_OFFSET;

    text_begin = .;
    .text : {
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
      . = ALIGN(THREAD_SIZE);
      main.o(.init_process_info)
      . = init_begin + THREAD_SIZE;
      init_end = .;
      *(.data)
    }
    data_end = .;

    vector_table_begin = .;
    .vector_table : {
      asm/main.o(.vector_table)
    }
    vector_table_end = .;

    bss_begin = .;
    .bss : {
      *(.bss)
      *(COMMON)
    }
    bss_end = .;
}
