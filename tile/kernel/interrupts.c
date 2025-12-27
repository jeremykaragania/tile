#include <kernel/interrupts.h>
#include <drivers/gic_400.h>
#include <drivers/pl011.h>
#include <drivers/sp804.h>
#include <kernel/buffer.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/syscall.h>

/*
  handle_fault handles a fault on address "addr". This fault can either be
  caused by a data abort or a prefetch abort. If the faulting address is
  mapped, then the containing page is demand paged in.
*/
int handle_fault(uint32_t addr) {
  struct list_link* pages_head = &current->mem->pages_head;
  struct page_region* region;
  struct file_info_int* file_int;
  struct buffer_info* buffer;
  void* phys_addr;

  region = find_page_region(pages_head, addr);

  if (!region) {
    return -1;
  }

  file_int = region->file_int;

  if (!file_int) {
    return -1;
  }

  /* The page region is backed by a file. */
  if (file_int) {
    uint32_t offset = addr - region->begin;
    struct filesystem_addr addr = file_offset_to_addr(file_int, offset);

    buffer = buffer_get(addr.num);
    phys_addr = virt_to_phys(buffer->data);
  }

  if (!phys_addr) {
    return -1;
  }

  create_mapping(addr, (uint32_t)phys_addr, PAGE_SIZE, region->flags);

  return 0;
}

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
void do_supervisor_call() {
  uint32_t number = get_syscall_number();
  int ret;

  ret = do_syscall(number);
  current->reg.r0 = ret;
}

/*
  do_prefetch_abort handles the prefetch abort exception.
*/
void do_prefetch_abort() {
  uint32_t ifar = get_ifar();

  if (handle_fault(ifar) < 0) {
    panic("");
  }
}

/*
  do_data_abort handles the data abort exception. It tries to allocate and map
  a physical page to the faulting virtual page.
*/
void do_data_abort() {
  uint32_t dfar = get_dfar();

  if (handle_fault(dfar) < 0) {
    panic("");
  }
}

/*
  do_irq handles the IRQ exception.
*/
void do_irq() {
  uint32_t ia = gicc->ia;

  gic_disable_interrupt(ia);

  switch (ia & GICC_IAR_INT_ID_MASK) {
    case TIM01INT:
      timer_0->timer1_int_clr = 0;
      schedule_tick();
      break;
    case UART0INTR:
      do_uart_irq(&uart);
      break;
    default:
      break;
  }

  /* Signal interrupt processing completion. */
  gicc->eoi = ia & GICC_IAR_INT_ID_MASK;
  gic_enable_interrupt(ia);
}

/*
  do_fiq handles the FIQ exception.
*/
void do_fiq() {}
