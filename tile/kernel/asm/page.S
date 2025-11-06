#include <kernel/asm/page.h>

.global invalidate_entire_tlb
invalidate_entire_tlb:
  mcr p15, 0, r0, c8, c6, 0
  bx lr

.global set_pgd
set_pgd:
  mcr p15, 0, r0, c2, c0, 0
  push {lr}
  bl invalidate_entire_tlb
  pop {lr}
  bx lr
