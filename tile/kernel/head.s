.text
.global _start
_start:
  ldr sp, =0x80200000
  bl start_kernel
1:
  b 1b
