#include <asm/page.h>

.section .text
.global pgd_index
pgd_index:
  ldr r1, =PG_DIR_SHIFT
  lsr r0, r0, r1
  mov r1, #0x4
  mul r0, r0, r1
  bx lr
