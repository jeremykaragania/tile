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
1:
  ldr r3, =#0xc02
  orr r4, r0, r3
  str r4, [r2]
  add r0, r0, #0x100000
  add r2, r2, #0x4
  ldr r5, =KERNEL_RAM_PADDR
  cmp r2, r5
  ble 1b
  mov r0, #0x3
  mcr p15, #0x0, r0, c3, c0, #0x0
  mcr p15, #0x0, r0, c1, c0, #0x0
  ldr sp, =KERNEL_RAM_PADDR
  bl start_kernel
2:
  b 2b
