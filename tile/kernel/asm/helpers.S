.section .text
/*
  The run-time ABI requires that some compiler helper functions return their
  struct members in registers. By passing the desired return values to a tail
  call of this function, this requirement can be emulated, since by calling
  this function, the return values will be placed in the appropriate registers.
*/
.global value_in_regs
value_in_regs:
  bx lr
