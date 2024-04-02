#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define STACK_END_MAGIC 0x57ac6e9d
#define THREAD_SIZE 0x00002000

extern void* init_process_data_begin;
extern void* init_process_data_end;

extern struct process_info init_process_info;
extern uint32_t current_stack_pointer();

enum process_state  {
  CREATED,
  READY,
  RUNNING,
  BLOCKED,
  TERMINATED
};

struct process_info {
  int id;
  int state;
  void* stack;
};

void set_process_stack_end_token(const struct process_info* proc);
struct process_info* current_process_info();

#endif
