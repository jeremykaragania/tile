#include <kernel/interrupts.h>
#include <drivers/gic_400.h>
#include <drivers/sp804.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/schedule.h>

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
  uint32_t* pmd = addr_to_pmd(current->mem->pgd, dfar);
  struct list_link* pages_head = &current->mem->pages_head;
  struct page_region* region;

  region = find_page_region(pages_head, dfar);

  /* The page region isn't backed by a file. */
  if (!region->file_int) {
    pmd_insert(pmd, dfar, (uint32_t)page_group_alloc(page_groups, PHYS_OFFSET, 1, 1, 0), region->flags);
  }
}

/*
  do_irq_interrupt handles the IRQ interrupt exception.
*/
void do_irq_interrupt() {
  uint32_t ia = gicc->ia;

  gic_disable_interrupt(ia);

  switch (ia) {
    case TIM01INT:
      timer_0->timer1_int_clr = 0;
      break;
    default:
      break;
  }

  /* Signal interrupt processing completion. */
  gicc->eoi = ia;
  gic_enable_interrupt(ia);
  schedule();
}

/*
  do_fiq_interrupt handles the FIQ interrupt exception.
*/
void do_fiq_interrupt() {}
