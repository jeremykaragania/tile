.section .text
/*
  phys_to_virt returns a physical address from a virtual address.
*/
.global phys_to_virt
phys_to_virt:
  ldr r4, =VIRT_OFFSET
  ldr r5, =PHYS_OFFSET
  sub r4, r4, r5
  add r0, r0, r4
  bx lr
/*
  virt_to_phys returns a virtual address from a physical address.
*/
.global virt_to_phys
virt_to_phys:
  ldr r4, =VIRT_OFFSET
  ldr r5, =PHYS_OFFSET
  sub r4, r4, r5
  sub r0, r0, r4
  bx lr
