#include <process.h>

const extern uint32_t* SMC_VADDR;
const extern uint32_t* KERNEL_SPACE_VADDR;
const extern uint32_t* PG_DIR_VADDR;

extern void* init_end;
extern void enable_interrupts();
extern void disable_interrupts();

static uint32_t* pg_dir = (uint32_t*)&PG_DIR_VADDR;
static struct process_table_entry init_process_table_entry __attribute__((section(".init_process_table_entry"))) = {0, CREATED, &init_end};

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process_table_entry);
  return;
}
