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
