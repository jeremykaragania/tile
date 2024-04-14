#include <interrupts.h>

void do_reset() {
  return;
}

void __attribute__((interrupt ("UNDEF"))) do_undefined_instruction() {
  return;
}

void __attribute__((interrupt ("SWI"))) do_supervisor_call() {
  return;
}

void __attribute__((interrupt ("ABORT"))) do_prefetch_abort() {
  return;
}

void __attribute__((interrupt ("ABORT"))) do_data_abort() {
  return;
}

void __attribute__((interrupt ("IRQ"))) do_irq_interrupt() {
  return;
}

void __attribute__((interrupt ("FIQ"))) do_fiq_interrupt() {
  return;
}
