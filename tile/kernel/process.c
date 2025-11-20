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
  .uid = 0,
  .euid = 0,
  .file_num = 1,
  .file_tab = NULL,
  .mem = &init_memory_info,
  .stack = &init_process_stack
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

  num = next_process_number();

  /* Userspace processes have a unique virtual memory context. */
  if (type == PT_USER) {
    proc->mem = create_memory_info();
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
int process_exec(const char* name) {
  int fd;
  struct file_info_int* file;
  uint32_t file_size;
  void* buf;

  fd = file_open(name, O_RDWR);

  if (fd < 0) {
    return -1;
  }

  file = (&current->file_tab[fd])->file_int;
  file_size = file->ext.size;

  buf = memory_alloc(ALIGN(file_size, PAGE_SIZE));
  file_read(fd, buf, file_size);

  if (load_elf(buf) < 0) {
    return -1;
  }

  current->reg.cpsr = PM_USR;
  current->reg.sp = stack_end(current);

  return 0;
}

/*
  getpid returns the process ID of the calling process.
*/
int getpid() {
  return current->num;
}

/*
  getuid returns the real user ID of the calling process.
*/
int getuid() {
  return current->uid;
}

/*
  next_process_number increments the process number count and returns the new
  value.
*/
int next_process_number() {
  return ++process_num_count;
}

/*
  load_elf loads an ELF file into the userspace portion of the current process.
  It loads all of the loadable segments and sets the program counter to the
  entry point.
*/
int load_elf(const void* elf) {
  const struct elf_hdr* hdr;
  const struct elf_phdr* phdr;
  int flags;

  hdr = (struct elf_hdr*)elf;

  if (!is_elf_header_valid(hdr)) {
    return -1;
  }

  phdr = (struct elf_phdr*)((uint32_t)elf + hdr->e_phoff);

  for (uint32_t i = 0; i < hdr->e_phnum; ++i) {
    phdr = &phdr[i];

    /* We only care about PT_LOAD segments. */
    if (phdr->p_type != PT_LOAD) {
      continue;
    }

    flags = elf_segment_to_page_flags(phdr->p_flags);
    create_mapping(phdr->p_vaddr, virt_to_phys((uint32_t)elf + phdr->p_offset), phdr->p_memsz, flags);
  }

  current->reg.pc = hdr->e_entry;

  return 0;
}

/*
  is_elf_header_valid checks if the ELF file meets the basic and ARM-specific
  identification requirements, and if it meets the kernel's requirements for
  execution.
*/
bool is_elf_header_valid(const struct elf_hdr* hdr) {
  if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
      hdr->e_ident[EI_MAG1] != ELFMAG1 ||
      hdr->e_ident[EI_MAG2] != ELFMAG2 ||
      hdr->e_ident[EI_MAG3] != ELFMAG3) {
    return false;
  }

  if (hdr->e_type != ET_EXEC) {
    return false;
  }

  if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
    return false;
  }

  if (hdr->e_ident[EI_DATA] != ELFDATA2LSB && hdr->e_ident[EI_DATA] != ELFDATA2MSB) {
    return false;
  }

  if (hdr->e_machine != EM_ARM) {
    return false;
  }

  if (!hdr->e_phoff) {
    return false;
  }

  return true;
}

/*
  elf_segment_to_page_flags converts the ELF program header flags to page
  region flags.
*/
int elf_segment_to_page_flags(uint32_t flags) {
  int ret = 0;

  if (flags & PF_X) {
    ret |= PAGE_EXECUTE;
  }

  if (flags & PF_W) {
    ret |= PAGE_WRITE;
  }

  if (flags & PF_R) {
    ret |= PAGE_READ;
  }

  return ret;
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

/*
  create_memory_info creates a new empty memory context and returns it. It is
  used when a process does not wish to share the parent's memory context.
*/
struct memory_info* create_memory_info() {
  struct memory_info* mem = memory_alloc(sizeof(struct memory_info));

  mem->pgd = create_pgd();

  list_init(&mem->pages_head);
  create_page_region_bounds(&mem->pages_head);

  return mem;
}
