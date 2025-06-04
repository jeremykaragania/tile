#include <drivers/gic_400.h>
#include <drivers/pl011.h>
#include <drivers/pl180.h>
#include <drivers/sp804.h>
#include <kernel/asm/memory.h>
#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/interrupts.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/process.h>

char tile_banner[] = "Tile\n";

/*
  start_kernel sets up the kernel.
*/
void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  init_memory_manager((uint32_t*)phys_to_virt(PG_DIR_PADDR), &text_begin, &text_end, &data_begin, &data_end, &bss_begin, &bss_end);
  init_memory_map();
  init_bitmaps();
  update_memory_map();
  init_paging();
  uart_init();
  mci_init();
  gic_init();
  dual_timer_init();
  buffer_init();
  init_clock();
  filesystem_init();
  uart_printf(tile_banner);
  enable_interrupts();
  filesystem_put();
  while(1);
}
