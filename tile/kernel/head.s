.text
.global _start
_start:
  ldr sp, =KERNEL_RAM_PADDR
  bl start_kernel
1:
  b 1b
