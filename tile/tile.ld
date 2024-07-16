PHYS_OFFSET = 0x80000000;
VIRT_OFFSET = 0xc0000000;
TEXT_OFFSET = 0x00008000;
THREAD_SIZE = 0x00002000;
PAGE_SIZE = 0x00001000;

ENTRY(_start)

SECTIONS {
    . = VIRT_OFFSET + TEXT_OFFSET;

    /*
      Head text section.
    */
    .head.text ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.head.text) - VIRT_OFFSET) {
      asm/main.o(.text)
    }
    .turn_mmu_on ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.turn_mmu_on) - VIRT_OFFSET) {
      asm/main.o(.turn_mmu_on)
    }

    /*. = ALIGN(0x100000);*/

    /*
      Text section.
    */
    .text ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.text) - VIRT_OFFSET) {
      text_begin = .;
      main.o(.text)
      *(.text)
      text_end = .;
    }

    /*
      Data section.
    */
    .rodata ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.rodata) - VIRT_OFFSET) {
      data_begin = .;
      rodata_begin = .;
      *(.rodata)
      rodata_end = .;
    }

    .init_process ALIGN(THREAD_SIZE) : AT(PHYS_OFFSET + ADDR(.init_process) - VIRT_OFFSET) {
      init_process_begin = .;
      init_process_stack = .;
      *(.init_process)
      . = init_process_begin + THREAD_SIZE;
      init_process_end = .;
    }

    .data ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.data) - VIRT_OFFSET) {
      asm/main.o(.data)
      main.o(.data)
      *(.data)
      data_end = .;
    }

    /*
      Interrupt vector table.
    */
    .vector_table ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.vector_table) - VIRT_OFFSET) {
      vector_table_begin = .;
      *(.vector_table)
      vector_table_end = .;
      interrupts_begin = .;
      *(.interrupts)
      interrupts_end = .;
    }

    /*
      Block starting symbol section.
    */
    .bss ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.bss) - VIRT_OFFSET) {
      bss_begin = .;
      *(.bss)
      *(COMMON)
      . = ALIGN(PAGE_SIZE);
      bss_end = .;
    }
}
