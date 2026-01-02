/*
  syscall makes a system call specified by the system call number in r0 and
  arguments in r1-r5. The system call result is returned in r0.
*/
.global syscall
syscall:
  push {lr}
  mov r7, r0

  mov r0, r1
  mov r1, r2
  mov r2, r3
  mov r4, r5
  mov r5, r6

  svc #0
  pop {lr}
  bx lr
