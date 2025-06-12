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
  cps #0x11
  mov sp, r0
  /* IRQ. */
  cps #0x12
  mov sp, r0
  /* Abort. */
  cps #0x17
  mov sp, r0
  /* Undefined. */
  cps #0x1b
  mov sp, r0
  /* Supervisor. */
  cps #0x13
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
  restore_registers restores the registers from a struct processor_registers
  pointer stored in r0.
*/
.global restore_registers
restore_registers:
  ldmia r0, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc}
