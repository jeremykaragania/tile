#include <process.h>

extern void* init_end;
static struct process_table_entry init_process_table_entry __attribute__((section(".init_process_table_entry"))) = {0, CREATED, &init_end};

void start_kernel() {
  return;
}
