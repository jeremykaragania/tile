#include <kernel/asm/offsets.h>

.text
/*
  dispatch_syscall dispatches a syscall to the correct handler. The syscall
  number is passed in "r0". It assumes that the arguments have already been
  preserved in the current process's register information.
*/
.global dispatch_syscall
dispatch_syscall:
  push {lr}

  mov r0, r7
  ldr r8, =current_registers
  blx r8
  mov r8, r0

  ldr r0, [r8, #PR_R0_OFFSET]
  ldr r1, [r8, #PR_R1_OFFSET]
  ldr r2, [r8, #PR_R2_OFFSET]
  ldr r3, [r8, #PR_R3_OFFSET]
  ldr r4, [r8, #PR_R4_OFFSET]
  ldr r5, [r8, #PR_R5_OFFSET]
  ldr r6, [r8, #PR_R6_OFFSET]

  ldr r8, =syscall_table
  ldr r8, [r8, r7, lsl #2]
  blx r8

  pop {lr}
  bx lr
