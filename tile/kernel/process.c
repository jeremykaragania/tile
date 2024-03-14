#include <process.h>

void set_process_stack_end_token(const struct process_table_entry* proc) {
  *(uint32_t*)(proc + 0x1) = STACK_END_MAGIC;
}

struct process_table_entry* current_process_table_entry() {
  return (struct process_table_entry*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}
