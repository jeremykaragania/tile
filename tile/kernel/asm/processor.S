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

.global set_processor_mode
set_processor_mode:
  mrs r1, cpsr
  and r1, r1, #0xffffffe0
  orr r1, r1, r0
  msr cpsr, r1
  isb
  bx lr
