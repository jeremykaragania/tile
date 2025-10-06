#include <kernel/asm/processor.h>

/*
  enable_interrupts enables the I and F bits in the Current Program Status
  Register (CPSR).
*/
.global enable_interrupts
enable_interrupts:
  cpsie aif
  bx lr

/*
  disable_interrupts disables the I and F bits in the CPSR.
*/
.global disable_interrupts
disable_interrupts:
  cpsid aif
  bx lr

/*
  init_stack_pointers initializes the stack pointers for all the processor
  modes which interrupts can be taken to.
*/
.global init_stack_pointers
init_stack_pointers:
  ldr r0, =init_process_end - 0x8
  /* FIQ. */
  cps #PM_FIQ
  mov sp, r0
  /* IRQ. */
  cps #PM_IRQ
  mov sp, r0
  /* Abort. */
  cps #PM_ABT
  mov sp, r0
  /* Undefined. */
  cps #PM_UND
  mov sp, r0
  /* Supervisor. */
  cps #PM_SVC
  mov sp, r0
  bx lr

.global set_processor_mode
set_processor_mode:
  mrs r1, cpsr
  and r1, r1, #0xffffffe0
  orr r1, r1, r0
  msr cpsr, r1
  isb
  bx lr

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
