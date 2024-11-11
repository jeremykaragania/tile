#include <kernel/interrupts.h>

/*
  do_reset handles the reset exception.
*/
void do_reset() {}

/*
  do_undefined_instruction handles the undefined instruction exception.
*/
void __attribute__((interrupt ("UNDEF"))) do_undefined_instruction() {}

/*
  do_supervisor_call handles the supervisor call exception.
*/
void __attribute__((interrupt ("SWI"))) do_supervisor_call() {}

/*
  do_prefetch_abort handles the prefetch abort exception.
*/
void __attribute__((interrupt ("ABORT"))) do_prefetch_abort() {}

/*
  do_data_abort handles the data abort exception. It tries to allocate and map
  a physical page to the faulting virtual page.
*/
void do_data_abort() {
  uint32_t dfar = get_dfar();
  uint32_t pmd = pgd_offset(memory_manager.pgd, dfar);

  pmd_insert(pmd, dfar, (uint32_t)bitmap_alloc(&phys_bitmaps, phys_bitmaps.offset), BLOCK_RW);
}

/*
  do_irq_interrupt handles the IRQ interrupt exception.
*/
void __attribute__((interrupt ("IRQ"))) do_irq_interrupt() {}

/*
  do_fiq_interrupt handles the FIQ interrupt exception.
*/
void __attribute__((interrupt ("FIQ"))) do_fiq_interrupt() {}
