#include <kernel/process.h>

struct process_info* process_pool[PROCESS_POOL_SIZE];

struct process_info process_infos = {
  .prev = &init_process,
  .next = &init_process
};

size_t process_count;
int process_num_count;

struct process_info init_process __attribute__((section(".init_process"))) = {
  0,
  PT_KERNEL,
  PS_CREATED,
  1,
  1,
  NULL,
  (uint32_t*)phys_to_virt(PG_DIR_PADDR),
  {0,},
  &init_process_stack,
  &process_infos,
  &process_infos
};

/*
  process_clone creates a child process of type "type" from the function
  specified by "func" and returns its process number.
*/
int process_clone(int type, struct function_info* func) {
  int num = 0;
  int index;
  struct process_info* proc;

  index = next_process_pool_index();

  if (index < 0) {
    return -1;
  }

  /*
    a process's information and stack is stored in a buffer of THREAD_SIZE at
    the bottom is the process information and directly above it, is its stack.
  */
  proc = pages_alloc(page_count(THREAD_SIZE));

  if (!proc) {
    return -1;
  }

  *proc = *current;

  num = get_process_number();

  proc->num = num;
  proc->type = type;
  function_to_process(proc, func);

  process_pool[index] = proc;
  ++process_count;

  proc->stack = proc + 1;
  proc->reg.sp = (uint32_t)proc + THREAD_SIZE - 8;
  proc->reg.cpsr = PM_SVC;
  set_process_stack_end_token(proc);

  return num;
}

/*
  next_process_pool_index tries to find a free index in the process pool. A
  non-negative index is returned if one exists, otherwise, a negative result is
  returned.
*/
int next_process_pool_index() {
  for (size_t i = 0; i < PROCESS_POOL_SIZE; ++i) {
    if (!process_pool[i]) {
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
  *(uint32_t*)(proc->stack) = STACK_END_MAGIC;
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
  return &current->reg;
}

/*
  function_to_process returns a process through "proc" from the function
  "func". It initializes the processor context of "proc" using "func".
*/
void function_to_process(struct process_info* proc, struct function_info* func) {
  proc->reg.r0 = (uint32_t)func->arg;
  proc->reg.pc = (uint32_t)func->ptr;
  proc->reg.cpsr = PM_SVC;
}

/*
  process_push adds the process information "proc" the process information
  list.
*/
void process_push(struct process_info* proc) {
  proc->next = process_infos.next;
  proc->prev = &process_infos;
  process_infos.next->prev = proc;
  process_infos.next = proc;
}

/*
  process_remove removes the process information "proc" from the process
  information list.
*/
void process_remove(struct process_info* proc) {
  proc->next->prev = proc->prev;
  proc->prev->next = proc->next;
}
