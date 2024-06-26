#include <asm/page.h>

.section .text
.global pgd_index
pgd_index:
  ldr r1, =PG_DIR_SHIFT
  lsr r0, r0, r1
  mov r1, #0x4
  mul r0, r0, r1
  bx lr

.global pmd_index
pmd_index:
  ldr r1, =PAGE_MASK
  and r0, r0, r1
  ldr r1, =PAGE_SHIFT
  lsr r0, r0, r1
  mov r1, #0x4
  mul r0, r0, r1
  bx lr

.global invalidate_entire_tlb
invalidate_entire_tlb:
  mcr p15, 0, r0, c8, c6, 0
  bx lr
