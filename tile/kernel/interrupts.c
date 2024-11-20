#include <kernel/interrupts.h>

/*
  do_reset handles the reset exception.
*/
void do_reset() {}

/*
  do_undefined_instruction handles the undefined instruction exception.
*/
void do_undefined_instruction() {}

/*
  do_supervisor_call handles the supervisor call exception.
*/
void do_supervisor_call() {}

/*
  do_prefetch_abort handles the prefetch abort exception.
*/
void do_prefetch_abort() {}

/*
  do_data_abort handles the data abort exception. It tries to allocate and map
  a physical page to the faulting virtual page.
*/
void do_data_abort() {
  uint32_t dfar = get_dfar();
  uint32_t* pmd = addr_to_pmd(memory_manager.pgd, dfar);

  pmd_insert(pmd, dfar, (uint32_t)bitmap_alloc(&phys_bitmaps, phys_bitmaps.offset), BLOCK_RW);
}

/*
  do_irq_interrupt handles the IRQ interrupt exception.
*/
void do_irq_interrupt() {}

/*
  do_fiq_interrupt handles the FIQ interrupt exception.
*/
void do_fiq_interrupt() {}
