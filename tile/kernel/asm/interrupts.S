.section .vector_table, "ax"
.global vector_table
vector_table:
  b do_reset
  b do_undefined_instruction
  b do_supervisor_call
  b do_prefetch_abort
  b do_data_abort
  b .
  b do_irq_interrupt
  b do_fiq_interrupt
