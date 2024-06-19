#include <interrupts.h>

void do_reset() {
  uart_puts("interrupt: reset");
}

void __attribute__((section(".interrupts"), interrupt ("UNDEF"))) do_undefined_instruction() {
  uart_puts("interrupt: undefined instruction");
}

void __attribute__((section(".interrupts"), interrupt ("SWI"))) do_supervisor_call() {
  uart_puts("interrupt: supervisor call");
}

void __attribute__((section(".interrupts"), interrupt ("ABORT"))) do_prefetch_abort() {
  uart_puts("interrupt: prefetch abort");
}

void __attribute__((section(".interrupts"), interrupt ("ABORT"))) do_data_abort() {
  uart_puts("interrupt: data abort");
}

void __attribute__((section(".interrupts"), interrupt ("IRQ"))) do_irq_interrupt() {
  uart_puts("interrupt: irq interrupt");
}

void __attribute__((section(".interrupts"), interrupt ("FIQ"))) do_fiq_interrupt() {
  uart_puts("interrupt: fiq interrupt");
}
