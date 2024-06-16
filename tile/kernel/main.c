#include <asm/memory.h>
#include <memory.h>
#include <page.h>
#include <process.h>
#include <uart.h>

extern void enable_interrupts();
extern void disable_interrupts();

char tile_banner[] = "Tile\n";

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  init_memory_manager((uint32_t*)phys_to_virt(PG_DIR_PADDR), &text_begin, &text_end, &data_begin, &data_end, &bss_begin, &bss_end);
  init_memory_map();
  update_memory_map();
  init_paging();
  uart_init();
  uart_printf(tile_banner);
}
