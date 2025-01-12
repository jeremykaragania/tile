#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/asm/memory.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <stdint.h>

#define STACK_END_MAGIC 0x57ac6e9d
#define THREAD_SIZE 0x00002000

#define current current_process()

extern void* init_process_begin;
extern void* init_process_end;
extern void* init_process_stack;

extern struct process_info init_process;
extern uint32_t current_stack_pointer();

/*
  enum process_state represents the state of a process.
*/
enum process_state  {
  PS_CREATED,
  PS_READY,
  PS_RUNNING,
  PS_BLOCKED,
  PS_TERMINATED
};

/*
  struct process_info represents a process.
*/
struct process_info {
  int num;
  int state;
  uint32_t file_num;
  struct file_table_entry* file_tab;
  uint32_t* pgd;
  void* stack;
};

void set_process_stack_end_token(const struct process_info* proc);
struct process_info* current_process();

#endif
