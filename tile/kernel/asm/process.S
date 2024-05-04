.section .text
/*
  current_stack_pointer returns the contents of the stack pointer register.
*/
.global current_stack_pointer
current_stack_pointer:
  mov r0, sp
  bx lr
