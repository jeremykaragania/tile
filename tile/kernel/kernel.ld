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
      Initial section.
    */
    init_begin = .;
    . = ALIGN(PAGE_SIZE);
    .init.text : {
      init_text_begin = .;
      asm/main.o(.text)
      main.o(.text)
      init_text_end = .;
    }

    . = ALIGN(PAGE_SIZE);
    .init.data : {
      init_data_begin = .;
      asm/main.o(.data)
      main.o(.data)
      init_data_end = .;
    }
    init_end = .;

    /*
      Text section.
    */
    text_begin = .;
    . = ALIGN(PAGE_SIZE);
    .text : {
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

    . = ALIGN(PAGE_SIZE);
    .data : {
      init_process_data_begin = .;
      . = ALIGN(THREAD_SIZE);
      main.o(.init_process_info)
      . = init_process_data_begin + THREAD_SIZE;
      init_process_data_end = .;
      *(.data)
    }
    data_end = .;

    /*
      Interrupt vector table.
    */
    . = ALIGN(0x4);
    vector_table_begin = .;
    .vector_table : {
      asm/main.o(.vector_table)
    }
    vector_table_end = .;

    /*
      Block starting symbol section.
    */
    .bss : {
      *(.bss)
      *(COMMON)
    }
}
