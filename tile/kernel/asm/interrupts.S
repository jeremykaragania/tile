#include <kernel/asm/processor.h>

.section .vector_table, "ax"
.global vector_table
vector_table:
  b do_reset_stub
  b do_undefined_instruction_stub
  b do_supervisor_call_stub
  b do_prefetch_abort_stub
  b do_data_abort_stub
  b .
  b do_irq_stub
  b do_fiq_stub

/*
  When we return from any of these interrupts, we want the state to be the same
  as it was before entering. We try to handle all interrupts in supervisor mode
  which means that any banked registers in other modes are just scratch
  registers.
*/
.section .interrupts, "x"
/*
  interrupt_stub generates an interrupt handler stub for the interrupt named
  "name". When an interrupt stub is entered, the return address is calculated
  as an offset "adjustment" from the link register. Interrupt are handled
  differently depending on whether it was taken from userspace or kernelspace.
*/
.macro interrupt_stub name adjustment
  /*
    Preserve the program counter and saved program status register on the
    supervisor mode stack.
  */
  add lr, #\adjustment
  srsfd sp!, #PM_SVC

  /* Switch to supervisor mode */
  cps #PM_SVC

  /* Preserve the remaining registers. */
  push {r0 - lr}

  ldr r0, =disable_interrupts
  blx r0

  ldr r1, [sp, #PR_CPSR_OFFSET]
  and r0, r1, #0x1f
  eor r0, #PM_SVC
  cmp r0, #0
  beq 0f
  b 1f
/* From kernelspace. */
0:
  interrupt_stub_kernel \name
/* From userspace. */
1:
  interrupt_stub_user \name
.endm

/*
  interrupt_stub_kernel generates an interrupt handler when an interrupt was
  taken from kernelspace. The preserved registers will be on the stack.
*/
.macro interrupt_stub_kernel name
  ldr r0, =do_\name
  blx r0

  mov r0, sp
  ldr r1, =ret_from_interrupt_kernel
  blx r1
.endm

/*
  interrupt_stub_user generates an interrupt handler when an interrupt was
  taken from userspace. Here, we preserve the userspace registers in the
  process information structure.
*/
.macro interrupt_stub_user name
  ldr r0, =current_registers
  blx r0
  ldr r1, =preserve_registers
  blx r1

  // We don't need the registers on the stack anymore.
  add sp, #68

  ldr r0, =do_\name
  blx r0

  ldr r0, =ret_from_interrupt_user
  bx r0
.endm

do_reset_stub:
  interrupt_stub name="reset" adjustment=0

do_undefined_instruction_stub:
  interrupt_stub name="undefined_instruction" adjustment=0

do_supervisor_call_stub:
  interrupt_stub name="supervisor_call" adjustment=0

do_prefetch_abort_stub:
  interrupt_stub name="prefetch_abort" adjustment=-4

do_data_abort_stub:
  interrupt_stub name="data_abort" adjustment=-8

do_irq_stub:
  interrupt_stub name="irq" adjustment=-4

do_fiq_stub:
  interrupt_stub name="fiq" adjustment=-4

.section .text
/*
  get_dfar returns the Data Fault Address Register: the address which caused a
  data abort.
*/
.global get_dfar
get_dfar:
  mrc p15, 0, r0, c6, c0, 0
  bx lr

/*
  get_ifar returns the Instruction Fault Address Register: the address which
  caused a prefetch abort.
*/
.global get_ifar
get_ifar:
  mrc p15, 0, r0, c6, c0, 2
  bx lr

/*
  ret_from_interrupt_kernel returns from an interrupt. It restores the
  preserved registers in the struct process_registers in r0.
*/
.global ret_from_interrupt_kernel
ret_from_interrupt_kernel:
  push {r0}
  bl schedule
  pop {r0}
  ldmia r0, {r0 - lr}
  /*
    This works since one of the first thing we do when entering an exception is
    execute the save return state instruction to the supervisor stack and
    that's the stack pointer which is preserved.
  */
  rfefd sp!

/*
  ret_from_interrupt_user returns from an interrupt to userspace, restoring
  registers from the preserved registers of the current process.
*/
.global ret_from_interrupt_user
ret_from_interrupt_user:
  push {r0}
  bl schedule
  pop {r0}
  bl current_registers
  mov lr, r0
  ldmia lr, {r0, r12}
  add lr, #PR_SP_OFFSET
  ldmia lr, {sp, lr}^
  add lr, #8
  rfefd lr

/*
  preserve_registers preserves userspace registers into a process's register
  information. The destination is passed in r0. It's basically just a memcpy
  but from the stack pointer.
*/
preserve_registers:
  mov r2, #0
preserve:
  ldr r4, [sp, r2, lsl #2]
  str r4, [r0, r2, lsl #2]
  add r2, #1
  cmp r2, #17
  ble preserve
  bx lr
