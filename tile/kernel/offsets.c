#include <kernel/processor.h>
#include <stddef.h>

#define DEFINE(symbol, expression) __asm__("\n.ascii \"" #symbol " %0\""  : : "i" (expression))

int main() {
  /* Processor register offsets. */
  DEFINE(PR_R0_OFFSET, offsetof(struct processor_registers, r0));
  DEFINE(PR_R1_OFFSET, offsetof(struct processor_registers, r1));
  DEFINE(PR_R2_OFFSET, offsetof(struct processor_registers, r2));
  DEFINE(PR_R3_OFFSET, offsetof(struct processor_registers, r3));
  DEFINE(PR_R4_OFFSET, offsetof(struct processor_registers, r4));
  DEFINE(PR_R5_OFFSET, offsetof(struct processor_registers, r5));
  DEFINE(PR_R6_OFFSET, offsetof(struct processor_registers, r6));
  DEFINE(PR_R7_OFFSET, offsetof(struct processor_registers, r7));
  DEFINE(PR_R8_OFFSET, offsetof(struct processor_registers, r8));
  DEFINE(PR_R9_OFFSET, offsetof(struct processor_registers, r9));
  DEFINE(PR_R10_OFFSET, offsetof(struct processor_registers, r10));
  DEFINE(PR_R11_OFFSET, offsetof(struct processor_registers, r11));
  DEFINE(PR_R12_OFFSET, offsetof(struct processor_registers, r12));
  DEFINE(PR_SP_OFFSET, offsetof(struct processor_registers, sp));
  DEFINE(PR_LR_OFFSET, offsetof(struct processor_registers, lr));
  DEFINE(PR_PC_OFFSET, offsetof(struct processor_registers, pc));
  DEFINE(PR_CPSR_OFFSET, offsetof(struct processor_registers, cpsr));

  /* Context register offsets */
  DEFINE(CR_R4_OFFSET, offsetof(struct context_registers, r4));
  DEFINE(CR_R5_OFFSET, offsetof(struct context_registers, r5));
  DEFINE(CR_R6_OFFSET, offsetof(struct context_registers, r6));
  DEFINE(CR_R7_OFFSET, offsetof(struct context_registers, r7));
  DEFINE(CR_R8_OFFSET, offsetof(struct context_registers, r8));
  DEFINE(CR_R10_OFFSET, offsetof(struct context_registers, r10));
  DEFINE(CR_R11_OFFSET, offsetof(struct context_registers, r11));
  DEFINE(CR_SP_OFFSET, offsetof(struct context_registers, sp));
  DEFINE(CR_PC_OFFSET, offsetof(struct context_registers, pc));
}
