#include <kernel/process.h>

struct process_info* process_table[PROCESS_TABLE_SIZE];

int process_num_count;

struct process_info init_process __attribute__((section(".init_process"))) = {
  0,
  PT_KERNEL,
  PS_CREATED,
  1,
  1,
  NULL,
  (uint32_t*)phys_to_virt(PG_DIR_PADDR),
  {{0}},
  &init_process_stack
};

/*
  process_clone creates a child process of type "type" from the function
  specified by "func" and returns its process number.
*/
int process_clone(int type, struct function_info* func) {
  int num = 0;
  int index;
  struct process_info* proc;
  void* stack;

  index = get_process_table_index();

  if (index < 0) {
    return -1;
  }

  num = get_process_number();

  proc = process_info_dup(current);

  if (!proc) {
    return -1;
  }

  proc->num = num;
  proc->type = type;
  function_to_process(proc, func);

  stack = pages_alloc(page_count(THREAD_SIZE));

  if (!stack) {
    process_info_free(proc);
    return -1;
  }

  process_table[index] = proc;
  proc->stack = stack;
  proc->processor.reg.sp = stack;
  set_process_stack_end_token(proc);

  return num;
}

/*
  get_process_table_index tries to find a free index in the process table. A
  non-negative index is returned if one exists, otherwise, a negative result is
  returned.
*/
int get_process_table_index() {
  for (size_t i = 0; i < PROCESS_TABLE_SIZE; ++i) {
    if (!process_table[i]) {
      return i;
    }
  }

  return -1;
}

/*
  get_process_number increments the process number count and returns the new
  value.
*/
int get_process_number() {
  return ++process_num_count;
}

/*
  set_process_stack_end_token receives a struct process_info "proc", and adds a
  magic token to the end of its stack. This allows us to detect the last usable
  uint32_t of the stack.
*/
void set_process_stack_end_token(const struct process_info* proc) {
  *(uint32_t*)(&proc->stack) = STACK_END_MAGIC;
}

/*
  current_process returns the current struct process in use. A struct
  process_info is always to a THREAD_SIZE boundary. From the current stack
  pointer, we can find the related struct process.
*/
struct process_info* current_process() {
  return (struct process_info*) (current_stack_pointer() & ~(THREAD_SIZE - 1));
}

/*
  current_registers returns the current process' register information. It
  exists as a helper to make it easier to access the field from assembly.
*/
struct processor_registers* current_registers() {
  return &current->processor.reg;
}

/*
  function_to_process returns a process through "proc" from the function
  "func". It initializes the processor context of "proc" using "func".
*/
void function_to_process(struct process_info* proc, struct function_info* func) {
  proc->processor.reg.r0 = (uint32_t)func->arg;
  proc->processor.reg.pc = (uint32_t)func->ptr;
  proc->processor.mode = PM_SVC;
}

/*
  process_info_dup duplicates the process specified by "proc" and returns a
  pointer to it. The pointer can the be freed by calling process_info_free.
*/
struct process_info* process_info_dup(struct process_info* proc) {
  struct process_info* ret;

  ret = process_info_alloc();

  if (!ret) {
    return NULL;
  }

  *ret = *proc;
  return ret;
}

/*
  process_info_alloc allocates process information and returns a pointer to it.
*/
struct process_info* process_info_alloc() {
  return memory_alloc(sizeof(struct process_info));
}

/*
  process_info_free frees teh process information specified by "proc".
*/
int process_info_free(struct process_info* proc) {
  return memory_free(proc);
}
