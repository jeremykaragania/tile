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
#include <kernel/schedule.h>

char tile_banner[] = "Tile\n";

int user_init() {
  return 0;
}

int kernel_init() {
  return 0;
}

void init_processes() {
  struct function_info user = {
    &user_init,
    NULL
  };

  struct function_info kernel = {
    &kernel_init,
    NULL
  };

  process_clone(PT_KERNEL, &user);
  process_clone(PT_KERNEL, &kernel);
}

/*
  start_kernel sets up the kernel.
*/
void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  memory_manager_init((uint32_t*)phys_to_virt(PG_DIR_PADDR), &text_begin, &text_end, &data_begin, &data_end, &bss_begin, &bss_end);
  initmem_init();
  bitmaps_init();
  update_memory_map();
  memory_alloc_init();
  init_paging();
  uart_init();
  mci_init();
  gic_init();
  dual_timer_init();
  buffer_init();
  init_clock();
  filesystem_init();
  uart_printf(tile_banner);
  schedule_init();
  init_processes();
  enable_interrupts();
  filesystem_put();
  while(1);
}
