#include <kernel/process.h>

struct process_info process_table[PROCESS_TABLE_SIZE];

struct process_info init_process __attribute__((section(".init_process"))) = {
  0,
  PT_KERNEL,
  PS_CREATED,
  1,
  1,
  NULL,
  (uint32_t*)phys_to_virt(PG_DIR_PADDR),
  {{0}},
  &init_process_stack
};

/*
  set_process_stack_end_token receives a struct process_info "proc", and adds a
  magic token to the end of its stack. This allows us to detect the last usable
  uint32_t of the stack.
*/
void set_process_stack_end_token(const struct process_info* proc) {
  *(uint32_t*)(proc->stack) = STACK_END_MAGIC;
}

/*
  current_process returns the current struct process in use. A struct
  process_info is always to a THREAD_SIZE boundary. From the current stack
  pointer, we can find the related struct process.
*/
struct process_info* current_process() {
  return (struct process_info*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}

/*
  function_to_process returns a process through "proc" from the function
  "func". It initializes the processor context of "proc" using "func".
*/
void function_to_process(struct process_info* proc, struct function_info* func) {
  proc->processor.reg.r0 = (uint32_t)func->arg;
  proc->processor.reg.pc = (uint32_t)func->ptr;
  proc->processor.mode = PM_SVC;
}
