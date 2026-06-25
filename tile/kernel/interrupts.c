#include <kernel/interrupts.h>
#include <drivers/gic_400.h>
#include <drivers/pl011.h>
#include <drivers/sp804.h>
#include <kernel/buffer.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/syscall.h>

/*
  handle_fault handles a fault on address "addr". This fault can either be
  caused by a data abort or a prefetch abort. If the faulting address is
  mapped, then the containing page is demand paged in.
*/
int handle_fault(uint32_t addr) {
  struct memory_info* mem;
  struct page_region* region;
  int ret;

  if (addr == 0) {
    return -1;
  }

  mem = current->mem;
  region = find_page_region(mem, addr);

  if (!region) {
    return -1;
  }

  switch (region->type) {
    case PR_FILE:
      ret = handle_file_fault(addr, region);
      break;
    default:
      ret = -1;
      break;
  }

  return ret;
}

int handle_anon_fault(uint32_t addr, struct page_region* region) {
  struct memory_info* mem;
  size_t index;
  struct phys_page* page;
  uint64_t page_addr;
  void* retval;

  mem = current->mem;

  index = (addr - region->begin) >> PAGE_SHIFT;
  page = pages_alloc(1, ZONE_HIGHMEM);

  if (!page) {
    return -1;
  }

  page_addr = page_to_addr(page);

  retval = create_mapping(mem, ALIGN_DOWN(addr, PAGE_SIZE), page_addr, PAGE_SIZE, region->flags);

  if (!retval) {
    return -1;
  }

  return 0;
}

/*
  handle_file_fault handles a fault on address "addr" which exists in the page
  region "region" which is file-backed.
*/
int handle_file_fault(uint32_t addr, struct page_region* region) {
  struct memory_info* mem;
  struct file_info_int* file;
  uint32_t offset;
  struct filesystem_addr fs_addr;
  struct buffer_info* buffer;
  uint64_t phys_addr;
  void* retval;

  mem = current->mem;
  file = region->file;

  if (!file) {
    return -1;
  }

  offset = region->file_offset + (addr - region->begin);
  fs_addr = file_offset_to_addr(file, offset);

  buffer = buffer_get(fs_addr.num);
  phys_addr = virt_to_phys((uint32_t)buffer->data);

  if (!phys_addr) {
    return -1;
  }

  retval = create_mapping(mem, ALIGN_DOWN(addr, PAGE_SIZE), phys_addr, PAGE_SIZE, region->flags);

  if (!retval) {
    return -1;
  }

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

  enable_interrupts();
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
