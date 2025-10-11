/*
  context_switch preserves the registers in r0 and restores the registers in
  r1, effectively performing a context switch.
*/
.global context_switch
context_switch:
  // Preserve registers.
  push {r0, r1}

  stmia r0, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr}
  str lr, [r0, #60] // Offset of PC in processor_registers.

  mrs r1, cpsr
  str r1, [r0, #64]

  pop {r0, r1}

  // Restore registers.
  ldr r0, [r1, #64] // Offset of CPSR in processor_registers.
  msr cpsr, r0
  ldmia r1, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc}
