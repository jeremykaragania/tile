#include <kernel/asm/page.h>

.global invalidate_entire_tlb
invalidate_entire_tlb:
  mcr p15, 0, r0, c8, c6, 0
  bx lr
