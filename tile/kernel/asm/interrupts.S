.section .vector_table, "ax"
.global vector_table
vector_table:
  b do_reset_stub
  b do_undefined_instruction_stub
  b do_supervisor_call_stub
  b do_prefetch_abort_stub
  b do_data_abort_stub
  b .
  b do_irq_interrupt_stub
  b do_fiq_interrupt_stub

/*
  When we return from any of these interrupts, we want the state to be the same
  as it was before entering. We try to handle all interrupts in supervisor mode
  which means that any banked registers in other modes are just scratch
  registers. For now, It's assumed that these interrupts are entered from
  supervisor mode.
*/
.section .interrupts, "x"
do_reset_stub:
  ldr r0, =do_reset
  bx r0

do_undefined_instruction_stub:
  ldr r0, =do_undefined_instruction
  bx r0

do_supervisor_call_stub:
  ldr r0, =do_supervisor_call
  bx r0

do_prefetch_abort_stub:
  ldr r0, =do_prefetch_abort
  bx r0

do_data_abort_stub:
  /*
    We're in abort mode. "lr" is the return address which we must decrement to
    return to the instruction which triggered the exception. We push this
    return address and the SPSR to the supervisor mode stack.
  */
  sub lr, #8
  srsfd sp!, #0x13

  /* Switch to supervisor mode */
  cps #0x13

  /* Preserve the supervisor mode state. */
  push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, sp, lr}
  ldr r0, =current_registers
  blx r0
  mov r1, #4
  ldr r2, =preserve_registers
  blx r2

  ldr r0, =do_data_abort
  blx r0

  /* Restore the supervisor mode state and return. */
  pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip}
  add sp, #4
  pop {lr}
  rfefd sp!

do_irq_interrupt_stub:
  /*
    We're in IRQ mode. "lr" is the return address which we must decrement to
    return to the instruction which triggered the exception. We push this
    return address and the SPSR to the supervisor mode stack.
  */
  sub lr, #4
  srsfd sp!, #0x13

  /* Switch to supervisor mode */
  cps #0x13

  /* Preserve the supervisor mode state. */
  push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, sp, lr}
  ldr r0, =current_registers
  blx r0
  mov r1, #4
  ldr r2, =preserve_registers
  blx r2

  ldr r0, =do_irq_interrupt
  blx r0

  /* Restore the supervisor mode state and return. */
  pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip}
  add sp, #4
  pop {lr}
  rfefd sp!

do_fiq_interrupt_stub:
  ldr r0, =do_fiq_interrupt
  bx r0

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
  preserve_registers preserves the processor's registers before a C exception
  handler is called. It assumes that all core registers have been pushed to the
  stack in order. It must adjust the stack pointer and program counter to
  account for the adjustments made in the exception handler stub. The
  preservation destination is stored in r0 and the amount the program counter
  must be adjusted is stored in r1.
*/
preserve_registers:
  mov r2, #0
  ldr r4, [sp, r2, lsl #2]
  str r4, [r0, r2, lsl #2]
  add r2, #1
  cmp r2, #17
  ble preserve_registers+24

  /* Adjust the preserved stack pointer and program counter.*/
  mov r2, #13 /* Stack pointer register number. */
  ldr r4, [r0, r2, lsl #2]
  sub r4, #8
  str r4, [r0, r2, lsl #2]

  mov r2, #15 /* Program counter register number. */
  ldr r4, [r0, r2, lsl #2]
  add r4, r1
  str r4, [r0, r2, lsl #2]

  bx lr
