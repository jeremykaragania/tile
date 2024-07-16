#include <kernel/interrupts.h>

void do_reset() {}

void __attribute__((interrupt ("UNDEF"))) do_undefined_instruction() {}

void __attribute__((interrupt ("SWI"))) do_supervisor_call() {}

void __attribute__((interrupt ("ABORT"))) do_prefetch_abort() {}

void __attribute__((interrupt ("ABORT"))) do_data_abort() {}

void __attribute__((interrupt ("IRQ"))) do_irq_interrupt() {}

void __attribute__((interrupt ("FIQ"))) do_fiq_interrupt() {}
