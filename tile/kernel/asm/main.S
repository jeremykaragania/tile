#include <kernel/asm/memory.h>
#include <kernel/asm/page.h>

.section .turn_mmu_on, "ax"
turn_mmu_on:
  mrc p15, #0x0, r0, c1, c0, #0x0
  orr r0, r0, #0x1
  mcr p15, #0x0, r0, c1, c0, #0x0 // Set MMU enable.
  ldr r0, =mmap_switched
  bx r0
.section .text
pgd_index:
  ldr r1, =PG_DIR_SHIFT
  lsr r0, r0, r1
  mov r1, #0x4
  mul r0, r0, r1
  ldr r1, =PG_DIR_PADDR
  add r0, r0, r1
  bx lr
/*
  phys_to_virt returns a virtual address from a physical address "x".
*/
phys_to_virt:
  ldr r4, =VIRT_OFFSET
  ldr r5, =PHYS_OFFSET
  sub r4, r4, r5
  add r0, r0, r4
  bx lr
/*
  virt_to_phys returns a virtual address from a physical address.
*/
virt_to_phys:
  ldr r4, =VIRT_OFFSET
  ldr r5, =PHYS_OFFSET
  sub r4, r4, r5
  sub r0, r0, r4
  bx lr
.global _start
_start:
  cps #0x13 // Set processor mode to supervisor.
  b map_turn_mmu_on
/*
  new_sections inserts new similar section entries into the page table. r0 is
  the beginning entry index which is correlated with the beginning virtual
  address, r1 is bits [19:0] of a section, r2 is the beginning physical
  address, and r3 is the ending physical address.
*/
new_sections:
  orr r4, r2, r1
  str r4, [r0]
  add r2, r2, #0x100000
  add r0, r0, #0x4
  cmp r2, r3
  blt new_sections
  bx lr
/*
  map_turn_mmu_on maps the turn_mmu_on symbol. It creates a one-to-one mapping
  which spans 1MB.
*/
map_turn_mmu_on:
  ldr r0, =turn_mmu_on
  bl virt_to_phys
  mov r4, r0
  bl pgd_index
  ldr r1, =#0x402 // Read/write, only at PL1.
  mov r2, r4
  add r3, r4, #0x100000
  bl new_sections
/*
  map_kernel maps the kernel space. It creates a
  KERNEL_SPACE_PADDR-to-0xc0000000 mapping which spans 768MB.
*/
map_kernel:
  ldr r0, =KERNEL_SPACE_PADDR
  bl phys_to_virt
  bl pgd_index
  ldr r1, =#0x402 // Read/write, only at PL1.
  ldr r2, =KERNEL_SPACE_PADDR
  ldr r3, =KERNEL_SPACE_PADDR + 0x30000000
  bl new_sections
/*
  map_smc maps the static memory controller. It creates a
  SMC_CS3_PADDR-to-0xffc00000 mapping which spans 1MB.
*/
map_smc:
  ldr r0, =#0xffc00000
  bl pgd_index
  ldr r1, =#0x402
  ldr r2, =SMC_CS3_PADDR
  ldr r3, =SMC_CS3_PADDR + 0x100000
  bl new_sections
enable_mmu:
  ldr r0, =PG_DIR_PADDR
  mcr p15, 0, r0, c2, c0, 0 // Set translation table base 0 address.
  mov r0, #0x1
  mcr p15, 0, r0, c3, c0, 0 // Set domain access permision.
  b turn_mmu_on
mmap_switched:
  mrc p15, 0, r0, c1, c0, 0
  orr r0, r0, #0x2000
  mcr p15, 0, r0, c1, c0, 0 // Set vector base address.
  bl init_stack_pointers
  bl start_kernel
