.set OTHER_ADDR, 0x00000000
.set KERNEL_SPACE_ADDR, 0x80010000
.set USER_SPACE_ADDR, 0xc0000000
.set TEXT_OFFSET, 0x00008000
.set PG_DIR_SIZE, 0x00004000
.set PG_DIR_ADDR, KERNEL_SPACE_ADDR + PG_DIR_SIZE
.section .vector_table, "x"
.global exception_base_address
exception_base_address:
  b .
  b .
  b .
  b .
  b .
  b .
  b .
  b .
.section .text
.global _start
_start:
  cps #0x13
  b map_other
new_sections:
  orr r4, r2, r0
  str r4, [r1]
  add r2, r2, #0x100000
  add r1, r1, #0x4
  cmp r1, r3
  blt new_sections
  bx lr
map_other:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =PG_DIR_ADDR
  ldr r2, =OTHER_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x2000)
  bl new_sections
map_kernel:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =(PG_DIR_ADDR + 0x2000)
  ldr r2, =KERNEL_SPACE_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x3000)
  bl new_sections
map_user:
  ldr r0, =#0xc02 // Read/write, at any privilege level.
  ldr r1, =(PG_DIR_ADDR + 0x3000)
  ldr r2, =USER_SPACE_ADDR
  ldr r3, =(PG_DIR_ADDR + 0x4000)
  bl new_sections
setup:
  ldr r0, =PG_DIR_ADDR
  mcr p15, #0x0, r0, c2, c0, #0x0 // Set translation table base 0 address.
  mov r0, #0x1
  mcr p15, #0x0, r0, c3, c0, #0x0 // Set domain access permision.
  mrc p15, #0x0, r0, c12, c0, #0x0
  ldr r1, =#0x8000000
  orr r0, r0, r1
  mcr p15, #0x0, r0, c12, c0, #0x0 // Set vector base address.
  mrc p15, #0x0, r0, c1, c0, #0x0
  orr r0, r0, #0x1
  mcr p15, #0x0, r0, c1, c0, #0x0 // Set MMU enable.
  ldr sp, =USER_SPACE_ADDR
  bl start_kernel
4:
  b 4b
