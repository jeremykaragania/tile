#include <memory.h>
#include <process.h>

const extern uint32_t* SMC_VADDR;
const extern uint32_t* KERNEL_SPACE_VADDR;
const extern uint32_t* PG_DIR_VADDR;

const extern uint32_t* text_begin;
const extern uint32_t* text_end;
const extern uint32_t* data_begin;
const extern uint32_t* data_end;
const extern uint32_t* bss_begin;
const extern uint32_t* bss_end;

extern void* init_end;
extern void enable_interrupts();
extern void disable_interrupts();

static uint32_t* pg_dir = (uint32_t*)&PG_DIR_VADDR;

static struct process_info init_process_info __attribute__((section(".init_process_info"))) = {
  0,
  CREATED,
  &init_end
};

static struct memory_info init_memory_info = {
  (uint32_t)&text_begin,
  (uint32_t)&text_end,
  (uint32_t)&data_begin,
  (uint32_t)&data_end
};

void start_kernel() {
  disable_interrupts();
  set_process_stack_end_token(&init_process_info);
  return;
}
