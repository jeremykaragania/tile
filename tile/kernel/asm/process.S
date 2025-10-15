.section .text
/*
  ret_from_clone calls the function stored in r1 with the argument stored in
  r0. This is what eventually happens after a process is cloned.
*/
.global ret_from_clone
ret_from_clone:
  bl enable_interrupts
  blx r1
  b ret_from_interrupt_user

/*
  current_stack_pointer returns the contents of the stack pointer register.
*/
.global current_stack_pointer
current_stack_pointer:
  mov r0, sp
  bx lr
