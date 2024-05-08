PHYS_OFFSET = 0x80000000;
VIRT_OFFSET = 0xc0000000;
TEXT_OFFSET = 0x00008000;
THREAD_SIZE = 0x00002000;
PAGE_SIZE = 0x00001000;

ENTRY(_start)

SECTIONS {
    . = PHYS_OFFSET;
    . += TEXT_OFFSET;

    /*
      Text section.
    */
    text_begin = .;
    . = ALIGN(PAGE_SIZE);
    .text : {
      asm/main.o(.text)
      main.o(.text)
      *(.text)
    }
    text_end = .;

    /*
      Data section.
    */
    data_begin = .;
    . = ALIGN(PAGE_SIZE);
    .rodata : {
      rodata_begin = .;
      *(.rodata)
      rodata_end = .;
    }

    .init_process : {
      . = ALIGN(THREAD_SIZE);
      init_process_begin = .;
      init_process_stack = .;
      *(.init_process)
      . = init_process_begin + THREAD_SIZE;
      init_process_end = .;
    }

    .data : {
      . = ALIGN(PAGE_SIZE);
      asm/main.o(.data)
      main.o(.data)
      *(.data)
    }
    data_end = .;

    /*
      Interrupt vector table.
    */
    .vector_table : {
      . = ALIGN(PAGE_SIZE);
      vector_table_begin = .;
      asm/main.o(.vector_table)
    }
    vector_table_end = .;

    /*
      Block starting symbol section.
    */
    bss_begin = .;
    .bss : {
      *(.bss)
      *(COMMON)
    }
    . = ALIGN(PAGE_SIZE);
    bss_end = .;
}
