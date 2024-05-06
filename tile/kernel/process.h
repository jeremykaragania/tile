#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define STACK_END_MAGIC 0x57ac6e9d
#define THREAD_SIZE 0x00002000

extern void* init_process_begin;
extern void* init_process_end;
extern void* init_process_stack;

extern struct process init_process;
extern uint32_t current_stack_pointer();

enum process_state  {
  CREATED,
  READY,
  RUNNING,
  BLOCKED,
  TERMINATED
};

struct process {
  int id;
  int state;
  void* stack;
};

void set_process_stack_end_token(const struct process* proc);
struct process* current_process();

#endif
