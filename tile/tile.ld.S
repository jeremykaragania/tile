#include <kernel/asm/memory.h>
#include <kernel/asm/page.h>

ENTRY(_start)

SECTIONS {
    . = VIRT_OFFSET + TEXT_OFFSET;

    /*
      Head text section.
    */
    .head.text ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.head.text) - VIRT_OFFSET) {
      kernel/asm/main.o(.text)
    }
    .turn_mmu_on ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.turn_mmu_on) - VIRT_OFFSET) {
      kernel/asm/main.o(.turn_mmu_on)
    }

    /*
      Text section.
    */
    .text ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.text) - VIRT_OFFSET) {
      text_begin = .;
      kernel/main.o(.text)
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
      *(.init_process)
      init_process_stack = .;
      . = init_process_begin + THREAD_SIZE;
      init_process_end = .;
    }

    .data ALIGN(PAGE_SIZE) : AT(PHYS_OFFSET + ADDR(.data) - VIRT_OFFSET) {
      kernel/asm/main.o(.data)
      kernel/main.o(.data)
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
