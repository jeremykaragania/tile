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
    return address to the supervisor mode stack.
  */
  mrs sp, sp_svc
  sub lr, #8
  push {lr}

  /* Switch to supervisor mode */
  cps #0x13

  /*
    Ensure that the supervisor mode stack pointer accounts for the previous
    push in abort mode.
  */
  mrs sp, sp_abt

  /* Preserve the supervisor mode state. */
  push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

  /* Call the main interrupt handler. */
  ldr r0, =do_data_abort
  blx r0

  /* Restore the supervisor mode state and return. */
  pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}

do_irq_interrupt_stub:
  /*
    We're in IRQ mode. "lr" is the return address which we must decrement to
    return to the instruction which triggered the exception. We push this
    return address to the supervisor mode stack.
  */
  mrs sp, sp_svc
  sub lr, #4
  push {lr}

  /* Switch to supervisor mode */
  cps #0x13

  /*
    Ensure that the supervisor mode stack pointer accounts for the previous
    push in IRQ mode.
  */
  mrs sp, sp_irq

  /* Preserve the supervisor mode state. */
  push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}

  ldr r0, =do_irq_interrupt
  blx r0

  /* Restore the supervisor mode state and return. */
  pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}

do_fiq_interrupt_stub:
  ldr r0, =do_fiq_interrupt
  bx r0
