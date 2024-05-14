#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <uart.h>

void do_reset();
void do_undefined_instruction();
void do_supervisor_call();
void do_prefetch_abort();
void do_data_abort();
void do_irq_interrupt();
void do_fiq_interrupt();

#endif
