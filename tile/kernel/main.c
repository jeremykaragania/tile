#include <asm/memory.h>
#include <memory.h>
#include <page.h>
#include <process.h>
#include <uart.h>

extern void enable_interrupts();
extern void disable_interrupts();

char tile_banner[] = "Tile\n";

void map_smc() {
  uint32_t* pte;
  pte = pte_alloc(&memory_manager, 0Xffc00000);
  pte_insert(pte, 0xffc00000, 0x1c090000);
}

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process);
  init_memory_manager((uint32_t*)PG_DIR_PADDR, &text_begin, &text_end, &data_begin, &data_end);
  update_memory_map();
  init_memory_map();
  map_smc();
  uart_init();
  uart_printf(tile_banner);
}
