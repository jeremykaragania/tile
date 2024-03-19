#include <process.h>

/*
  set_process_stack_end_token receives a struct process_info "proc", and adds a magic token to the end of its stack.
  This allows us to detect the last usable uint32_t of the stack.
*/
void set_process_stack_end_token(const struct process_info* proc) {
  *(uint32_t*)(proc + 0x1) = STACK_END_MAGIC;
}

/*
  current_process_info returns the current struct process_info in use. A struct process_info is always to
  a THREAD_SIZE boundary. From the current stack pointer, we can find the related struct process_info.
*/
struct process_info* current_process_info() {
  return (struct process_info*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}
