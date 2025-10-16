#include <kernel/process.h>
#include <kernel/interrupts.h>
#include <kernel/asm/memory.h>
#include <kernel/asm/processor.h>
#include <kernel/file.h>
#include <kernel/memory.h>
#include <kernel/page.h>

struct list_link processes_head;

int process_num_count;

struct memory_info init_memory_info = {
  .pgd = (uint32_t*)phys_to_virt(PG_DIR_PADDR),
  .text_begin = (uint32_t)&text_begin,
  .text_end = (uint32_t)&text_end,
  .data_begin = (uint32_t)&data_begin,
  .data_end = (uint32_t)&data_end,
  .bss_begin = (uint32_t)&bss_begin,
  .bss_end = (uint32_t)&bss_end
};

struct process_info init_process __attribute__((section(".init_process"))) = {
  .num = 0,
  .type = PT_KERNEL,
  .state = PS_CREATED,
  .owner = 1,
  .file_num = 1,
  .file_tab = NULL,
  .mem = &init_memory_info,
  .stack = &init_process_stack,
};

/*
  process_clone creates a child process of type "type" from the function
  specified by "func" and returns its process number.
*/
int process_clone(int type, struct function_info* func) {
  int num = 0;
  struct process_info* proc;

  /*
    A process's information and stack is stored in a buffer of THREAD_SIZE at
    the bottom is the process information and directly above it, is its stack.
  */
  proc = memory_alloc(THREAD_SIZE);

  if (!proc) {
    return -1;
  }

  *proc = *current;

  num = get_process_number();

  /* Userspace processes have a unique virtual memory context. */
  if (type == PT_USER) {
    struct memory_info* mem = memory_alloc(sizeof(struct memory_info));

    if (!mem) {
      return -1;
    }

    proc->mem = mem;
  }

  proc->num = num;
  proc->type = type;
  function_to_process(proc, func);

  list_push(&processes_head, &proc->link);

  proc->stack = stack_begin(proc);
  proc->context_reg.sp = stack_end(proc);
  proc->context_reg.cpsr = PM_SVC;
  set_process_stack_end_token(proc);

  return num;
}

/*
  process_exec executes the file specified by its name "name". Currently there
  isn't an executable file format and this function supports instruction blobs.
*/
int process_exec(char* name) {
  int fd;
  void* addr;

  fd = file_open(name, O_RDWR);

  if (fd < 0) {
    return 0;
  }

  addr = file_map(fd, PAGE_RWX);

  if (!addr) {
    return 0;
  }

  current->reg.cpsr = PM_SVC;
  current->reg.sp = stack_end(current);
  current->reg.pc = (uint32_t)addr;
  return 1;
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
  current_context_registers returns the current process` context register
  information. It exists as a helper to make it easier to access the field from
  assembly.
*/
struct processor_registers* current_context_registers() {
  return &current->context_reg;
}

/*
  function_to_process returns a process through "proc" from the function
  "func". It initializes the processor context of "proc" using "func".
*/
void function_to_process(struct process_info* proc, struct function_info* func) {
  proc->context_reg.r0 = (uint32_t)func->arg;
  proc->context_reg.r1 = (uint32_t)func->ptr;
  proc->context_reg.pc = (uint32_t)ret_from_clone;
  proc->context_reg.cpsr = PM_SVC;
}
