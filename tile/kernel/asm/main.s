.set PG_DIR_SIZE, 0x00004000
.set SMC_PADDR, 0x00000000
.global KERNEL_SPACE_PADDR
.set KERNEL_SPACE_PADDR, 0x80000000
.set PG_DIR_PADDR, KERNEL_SPACE_PADDR + PG_DIR_SIZE
.global SMC_VADDR
.set SMC_VADDR, 0xe0000000
.global KERNEL_SPACE_VADDR
.set KERNEL_SPACE_VADDR, 0xc0000000
.global PG_DIR_VADDR
.set PG_DIR_VADDR, KERNEL_SPACE_VADDR + PG_DIR_SIZE
.section .vector_table, "x"
.global vector_table
vector_table:
  b .
  b .
  b .
  b .
  b .
  b .
  b .
  b .
.section .text
/*
  enable_interrupts enables the I and F bits in the Current Program Status Register (CPSR).
*/
.global enable_interrupts
enable_interrupts:
  cpsie if
  bx lr
/*
  disable_interrupts disables the I and F bits in the CPSR.
*/
.global disable_interrupts
disable_interrupts:
  cpsid if
  bx lr
.global _start
_start:
  cps #0x13 // Set processor mode to supervisor.
  b map_turn_mmu_on
/*
  new_sections inserts new similar section entries into the page table. r0 is bits [19:0] of a section, r1 is the beginning
  entry index which is correlated with the beginning virtual address, r2 is the beginning physical address, and r3 is the
  ending physical address.
*/
new_sections:
  orr r4, r2, r0
  str r4, [r1]
  add r2, r2, #0x100000
  add r1, r1, #0x4
  cmp r2, r3
  blt new_sections
  bx lr
/*
  map_turn_mmu_on maps the turn_mmu_on symbol. It creates a one-to-one mapping which spans 1MB.
*/
map_turn_mmu_on:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =turn_mmu_on
  lsr r1, r1, #20
  mov r4, #0x4
  mul r1, r1, r4
  ldr r4, =PG_DIR_PADDR
  add r1, r1, r4
  ldr r2, =turn_mmu_on
  lsr r2, r2, #19
  lsl r2, r2, #19
  ldr r3, =turn_mmu_on + 0x100000
  bl new_sections
/*
  map_kernel maps the kernel space. It creates a KERNEL_SPACE_PADDR-to-0xc0000000 mapping which spans 500MB.
*/
map_kernel:
  ldr r0, =#0x402 // Read/write, only at PL1.
  ldr r1, =PG_DIR_PADDR + (0xc0000000 >> 20) * 0x4
  ldr r2, =KERNEL_SPACE_PADDR
  ldr r3, =KERNEL_SPACE_PADDR + 0x20000000
  bl new_sections
/*
  map_smc maps the static memory controller. It creates a SMC_PADDR-to-0xe0000000 mapping which spans 500MB.
*/
map_smc:
  ldr r0, =#0x402
  ldr r1, =PG_DIR_PADDR + (0xe0000000 >> 20) * 0x4
  ldr r2, =SMC_PADDR
  ldr r3, =SMC_PADDR + 0x20000000
  bl new_sections
enable_mmu:
  ldr r0, =PG_DIR_PADDR
  mcr p15, #0x0, r0, c2, c0, #0x0 // Set translation table base 0 address.
  mov r0, #0x1
  mcr p15, #0x0, r0, c3, c0, #0x0 // Set domain access permision.
  b turn_mmu_on
turn_mmu_on:
  mrc p15, #0x0, r0, c1, c0, #0x0
  orr r0, r0, #0x1
  mcr p15, #0x0, r0, c1, c0, #0x0 // Set MMU enable.
  ldr r0, =mmap_switched
  bl phys_to_virt
  bx r0
mmap_switched:
  mrc p15, #0x0, r0, c12, c0, #0x0
  ldr r1, =vector_table_begin;
  orr r0, r0, r1
  mcr p15, #0x0, r0, c12, c0, #0x0 // Set vector base address.
  ldr r0, =mmap_switched
  bl phys_to_virt
  mov sp, r0
  bl start_kernel
4:
  b 4b
