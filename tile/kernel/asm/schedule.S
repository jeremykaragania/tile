#include <kernel/asm/processor.h>

/*
  context_switch preserves the registers in r0 and restores the registers in
  r1, effectively performing a context switch.
*/
.global context_switch
context_switch:
  // Preserve registers.
  stmia r0, {r4 - r8}
  str r10, [r0, #CR_R10_OFFSET]
  str r11, [r0, #CR_R11_OFFSET]
  str sp, [r0, #CR_SP_OFFSET]
  str lr, [r0, #CR_PC_OFFSET]

  // Restore registers.
  ldmia r1, {r4 - r8}
  ldr r10, [r1, #CR_R10_OFFSET]
  ldr r11, [r1, #CR_R11_OFFSET]
  ldr sp, [r1, #CR_SP_OFFSET]
  ldr pc, [r1, #CR_PC_OFFSET]
