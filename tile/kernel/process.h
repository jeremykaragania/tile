#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/list.h>
#include <kernel/processor.h>
#include <kernel/schedule.h>
#include <stdint.h>

#define STACK_END_MAGIC 0x57ac6e9d

#define stack_begin(proc) (proc + 1)
#define stack_end(proc) ((uint32_t)proc + THREAD_SIZE - 8)
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
  struct memory_info represents the virtual memory information of a process.
*/
struct memory_info {
  uint32_t* pgd;
  struct list_link pages_head;
  uint32_t text_begin;
  uint32_t text_end;
  uint32_t data_begin;
  uint32_t data_end;
  uint32_t bss_begin;
  uint32_t bss_end;
};

/*
  struct process_info represents a process.
*/
struct process_info {
  int num;
  int type;
  int state;
  int uid;
  int euid;
  uint32_t file_num;
  struct file_table_entry* file_tab;
  struct memory_info* mem;
  struct processor_registers reg;
  struct processor_registers context_reg;
  struct schedule_info sched;
  void* stack;
  struct list_link link;
};

const extern uint32_t* text_begin;
const extern uint32_t* text_end;
const extern uint32_t* data_begin;
const extern uint32_t* data_end;
const extern uint32_t* interrupts_begin;
const extern uint32_t* interrupts_end;
const extern uint32_t* bss_begin;
const extern uint32_t* bss_end;

extern void* init_process_begin;
extern void* init_process_end;
extern void* init_process_stack;

extern struct list_link processes_head;
extern int process_num_count;
extern struct memory_info init_memory_info;
extern struct process_info init_process;
extern uint32_t current_stack_pointer();
extern void ret_from_clone(int (*ptr)(void*), void* arg);

/*
  struct funciton_info represents a function which can be scheduled.
*/
struct function_info {
  int (*ptr)(void*);
  void* arg;
};

int process_clone(int type, struct function_info* func);
int process_exec(char* filename);
int get_process_number();

void set_process_stack_end_token(const struct process_info* proc);
struct process_info* current_process();
struct processor_registers* current_registers();
struct processor_registers* current_context_registers();
void function_to_process(struct process_info* proc, struct function_info* func);

struct memory_info* create_memory_info();

#endif
