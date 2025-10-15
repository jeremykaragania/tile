#include <kernel/asm/processor.h>

/*
  context_switch preserves the registers in r0 and restores the registers in
  r1, effectively performing a context switch.
*/
.global context_switch
context_switch:
  // Preserve registers.
  push {r0}

  stmia r0, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr}
  str lr, [r0, #PR_PC_OFFSET]

  // Adjust stack pointer to account for push.
  ldr r2, [r0, #PR_SP_OFFSET]
  add r2, #4
  str r2, [r0, #PR_SP_OFFSET]

  mrs r2, cpsr
  str r2, [r0, #PR_CPSR_OFFSET]

  pop {r0}

  // Restore registers.
  ldr r0, [r1, #PR_CPSR_OFFSET]
  msr cpsr, r0
  isb
  bl disable_interrupts
  ldmia r1, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc}
