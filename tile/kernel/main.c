#include <memory.h>
#include <process.h>
#include <uart.h>

extern void* init_process_data_end;
extern void enable_interrupts();
extern void disable_interrupts();

static struct process_info init_process_info __attribute__((section(".init_process_info"))) = {
  0,
  CREATED,
  &init_process_data_end
};

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process_info);
  uart_init();
  return;
}
