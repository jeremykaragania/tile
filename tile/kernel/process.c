#include <process.h>

struct process init_process __attribute__((section(".init_process"))) = {
  0,
  CREATED,
  &init_process_data_end
};

/*
  set_process_stack_end_token receives a struct process "proc", and adds a
  magic token to the end of its stack.  This allows us to detect the last
  usable uint32_t of the stack.
*/
void set_process_stack_end_token(const struct process* proc) {
  *(uint32_t*)(proc + 0x1) = STACK_END_MAGIC;
}

/*
  current_process returns the current struct process in use. A struct process
  is always to a THREAD_SIZE boundary. From the current stack pointer, we can
  find the related struct process.
*/
struct process* current_process() {
  return (struct process*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}
