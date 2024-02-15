.set OTHER_ADDR, 0x00000000
.set KERNEL_SPACE_ADDR, 0x80000000
.set USER_SPACE_ADDR, 0xc0000000
.set TEXT_OFFSET, 0x00008000
.set KERNEL_ADDR, KERNEL_SPACE_ADDR + TEXT_OFFSET
.set PG_DIR_SIZE, 0x00004000
.set PG_DIR_ADDR, KERNEL_SPACE_ADDR + PG_DIR_SIZE
.text
.global _start
_start:
  b map_other
new_sections:
  orr r4, r2, r0
  str r4, [r1]
  add r2, r2, #0x100000
  add r1, r1, #0x4
  cmp r1, r3
  blt sections
  bx lr
map_other:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =PG_DIR_ADDR
  ldr r2, =OTHER_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x2000)
  bl new_sections
map_user:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =(PG_DIR_ADDR + 0x2000)
  ldr r2, =KERNEL_SPACE_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x3000)
  bl new_sections
map_kernel:
  ldr r0, =#0xc02 // Read/write, at any privilege level.
  ldr r1, =(PG_DIR_ADDR + 0x3000)
  ldr r2, =USER_SPACE_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x4000)
  bl new_sections
  ldr sp, =KERNEL_ADDR
emable_mmu:
  ldr r0, =PG_DIR_ADDR
  mcr p15, #0x0, r0, c2, c0, #0x0
  mov r0, #0x3
  mcr p15, #0x0, r0, c3, c0, #0x0
  mrc p15, #0x0, r0, c1, c0, #0x0
  orr r0, r0, #0x20000001
  mcr p15, #0x0, r0, c1, c0, #0x0 // Enable access flag and MMU.
  bl start_kernel
4:
  b 4b
