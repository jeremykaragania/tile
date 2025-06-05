#include <kernel/process.h>

struct process_info process_table[PROCESS_TABLE_SIZE];

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
  struct process_info* proc;

  num = get_process_number();

  if (!num) {
    return -1;
  }

  proc = &process_table[num - 1];
  proc->num = num;
  proc->type = type;
  function_to_process(proc, func);

  return num;
}

/*
  get_process_number tries to find a free process number. A positive process
  number is returned if one exists, otherwise, a negative result is returned.
*/
int get_process_number() {
  for (size_t i = 0; i < PROCESS_TABLE_SIZE; ++i) {
    if (process_table[i].num == 0) {
      /* Process numbering begins at 1. */
      return i + 1;
    }
  }

  return -1;
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
