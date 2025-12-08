#include <drivers/gic_400.h>
#include <drivers/pl011.h>
#include <drivers/pl180.h>
#include <drivers/sp804.h>
#include <kernel/asm/memory.h>
#include <kernel/buffer.h>
#include <kernel/file.h>
#include <kernel/interrupts.h>
#include <kernel/list.h>
#include <kernel/log.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/process.h>
#include <kernel/schedule.h>

char tile_banner[] = "Tile\n";

int user_init() {
  if (process_exec("/sbin/init") < 0) {
    panic("");
  }

  return 0;
}

int kernel_init() {
  while(1);
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

  /* Open /dev/console for standard streams. */
  int fd = file_open("/dev/console", O_RDWR);

  file_dup(fd);
  file_dup(fd);

  process_clone(PT_KERNEL, &user);
  process_clone(PT_KERNEL, &kernel);
}

/*
  start_kernel sets up the kernel.
*/
void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  initmem_init();
  memory_alloc_init();
  update_memory_map();
  init_paging();
  uart_init();
  mci_init();
  gic_init();
  dual_timer_init();
  buffer_init();
  filesystem_init();
  log_printf(tile_banner);
  schedule_init();
  init_processes();
  enable_interrupts();
  while(1);
}
