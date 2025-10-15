#include <kernel/asm/processor.h>

/*
  enable_interrupts enables the I and F bits in the Current Program Status
  Register (CPSR).
*/
.global enable_interrupts
enable_interrupts:
  cpsie aif
  isb
  bx lr

/*
  disable_interrupts disables the I and F bits in the CPSR.
*/
.global disable_interrupts
disable_interrupts:
  cpsid aif
  isb
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
