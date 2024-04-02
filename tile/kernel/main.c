#include <memory.h>
#include <process.h>
#include <uart.h>

extern void enable_interrupts();
extern void disable_interrupts();

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process_info);
  uart_init();
  init_init_memory_manager_info(&PG_DIR_PADDR, &text_begin, &text_end, &data_begin, &data_end);
  return;
}
