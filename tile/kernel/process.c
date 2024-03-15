#include <process.h>

/*
  set_process_stack_end_token receives a struct process_table_entry "proc", and adds a magic token to the end of its stack.
  This allows us to detect the last usable uint32_t of the stack.
*/
void set_process_stack_end_token(const struct process_table_entry* proc) {
  *(uint32_t*)(proc + 0x1) = STACK_END_MAGIC;
}

/*
  current_process_table_entry returns the current struct process_table_entry in use. A struct process_table_entry is always to
  a THREAD_SIZE boundary. From the current stack pointer, we can find the related struct process_table_entry.
*/
struct process_table_entry* current_process_table_entry() {
  return (struct process_table_entry*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}
