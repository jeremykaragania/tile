#include <kernel/asm/page.h>

.global flush_pgd
flush_pgd:
  mcr p15, 0, r0, c8, c6, 0
  dsb
  isb
  bx lr

.global flush_pte
flush_pte:
  mcr p15, 0, r0, c8, c7, 3
  dsb
  isb
  bx lr

.global set_pgd
set_pgd:
  mcr p15, 0, r0, c2, c0, 0 // Set translation table base 0 address.
  mov r0, #0x1
  mcr p15, 0, r0, c3, c0, 0 // Set domain access permision.
  push {lr}
  bl flush_pgd
  pop {lr}
  bx lr
