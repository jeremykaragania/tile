#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define STACK_END_MAGIC 0x57ac6e9d
#define THREAD_SIZE 0x00002000

enum process_state  {
  CREATED,
  READY,
  RUNNING,
  BLOCKED,
  TERMINATED
};

struct process_table_entry {
  int id;
  int state;
  void* stack;
};

void set_process_stack_end_token(const struct process_table_entry* proc);

extern uint32_t current_stack_pointer();

struct process_table_entry* current_process_table_entry();

#endif
