.text
.global _start
_start:
  mov r7, #6 // write syscall number.
  mov r0, #1
  ldr r1, =buf
  mov r2, #14
  svc #0

  b .
buf:
  .asciz "Hello, World!\n"
