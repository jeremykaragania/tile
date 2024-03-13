#ifndef PROCESS_H
#define PROCESS_H

#define STACK_END_MAGIC 0x57ac6e9d

#include <stdint.h>

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

#endif
