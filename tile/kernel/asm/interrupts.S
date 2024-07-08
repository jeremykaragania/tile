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
  ldr r0, =do_data_abort
  bx r0

do_irq_interrupt_stub:
  ldr r0, =do_irq_interrupt
  bx r0

do_fiq_interrupt_stub:
  ldr r0, =do_fiq_interrupt
  bx r0
