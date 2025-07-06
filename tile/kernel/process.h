#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/asm/memory.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/processor.h>
#include <stdint.h>

#define PROCESS_POOL_SIZE 32
#define STACK_END_MAGIC 0x57ac6e9d

#define current current_process()

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
  enum process_type represents the type of a process. A process can either be a
  user process or a kernel process.
*/
enum process_type {
  PT_USER,
  PT_KERNEL
};

/*
  struct process_info represents a process.
*/
struct process_info {
  int num;
  int type;
  int state;
  int owner;
  uint32_t file_num;
  struct file_table_entry* file_tab;
  uint32_t* pgd;
  struct processor_registers reg;
  void* stack;
  struct process_info* prev;
  struct process_info* next;
};

extern void* init_process_begin;
extern void* init_process_end;
extern void* init_process_stack;

extern struct process_info* process_pool[PROCESS_POOL_SIZE];
extern struct process_info process_infos;
extern size_t process_count;
extern int process_num_count;
extern struct process_info init_process;
extern uint32_t current_stack_pointer();

/*
  struct funciton_info represents a function which can be scheduled.
*/
struct function_info {
  int (*ptr)(void*);
  void* arg;
};

int process_clone(int type, struct function_info* func);
int next_process_pool_index();
int get_process_number();

void set_process_stack_end_token(const struct process_info* proc);
struct process_info* current_process();
struct processor_registers* current_registers();
void function_to_process(struct process_info* proc, struct function_info* func);

void process_info_push(struct process_info* proc);
void process_remove(struct process_info* proc);

#endif
