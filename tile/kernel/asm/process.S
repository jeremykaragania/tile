#include <kernel/asm/memory.h>

.section .text

/*
  current_process returns the current struct process in use. A struct
  process_info is always to a THREAD_SIZE boundary. From the current stack
  pointer, we can find the related struct process.
*/
.global current_process
current_process:
  mov r0, sp
  mov r1, #THREAD_SIZE - 1
  bic r0, r0, r1
  bx lr

/*
  ret_from_clone calls the function stored in r1 with the argument stored in
  r0. This is what eventually happens after a process is cloned.
*/
.global ret_from_clone
ret_from_clone:
  bl enable_interrupts
  mov r0, r4
  blx r5
  b ret_from_interrupt_user

/*
  current_stack_pointer returns the contents of the stack pointer register.
*/
.global current_stack_pointer
current_stack_pointer:
  mov r0, sp
  bx lr
