.set PHYS_OFFSET, 0x80000000
.set TEXT_OFFSET, 0x00008000
.set KERNEL_RAM_PADDR, PHYS_OFFSET + TEXT_OFFSET
.set PG_DIR_SIZE, 0x00004000
.set PG_DIR_PADDR, PHYS_OFFSET + PG_DIR_SIZE
.text
.global _start
_start:
  ldr r0, =PG_DIR_PADDR
  mcr p15, #0x0, r0, c2, c0, #0x0
  mov r0, #0
  mov r1, #0
  ldr r2, =PG_DIR_PADDR
  ldr r3, =KERNEL_RAM_PADDR
1:
  ldr r4, =#0xc02
  orr r5, r0, r4
  str r5, [r2]
  add r0, r0, #0x100000
  add r2, r2, #0x4
  cmp r2, r3
  ble 1b
  mov r0, #0x3
  mcr p15, #0x0, r0, c3, c0, #0x0
  mrc p15, #0x0, r0, c1, c0, #0x0
  orr r0, r0, #0x1
  mcr p15, #0x0, r0, c1, c0, #0x0
  mov sp, r3
  bl start_kernel
2:
  b 2b
