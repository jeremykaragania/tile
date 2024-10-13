#include <drivers/pl011.h>
#include <drivers/pl180.h>
#include <kernel/asm/memory.h>
#include <kernel/interrupts.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/process.h>

extern void enable_interrupts();
extern void disable_interrupts();

char tile_banner[] = "Tile\n";

/*
  start_kernel sets up the kernel.
*/
void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  init_memory_manager((uint32_t*)phys_to_virt(PG_DIR_PADDR), &text_begin, &text_end, &data_begin, &data_end, &bss_begin, &bss_end);
  init_memory_map();
  update_memory_map();
  init_paging();
  uart_init();
  mci_init();
  filesystem_init();
  uart_printf(tile_banner);
}
