#include <memory.h>
#include <process.h>
#include <uart.h>

extern void enable_interrupts();
extern void disable_interrupts();

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process_info);
  init_memory_map();
  init_init_memory_manager(&PG_DIR_PADDR, &text_begin, &text_end, &data_begin, &data_end);
  uart_init();
  return;
}
