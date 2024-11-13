#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <drivers/pl011.h>
#include <kernel/memory.h>
#include <kernel/page.h>

extern uint32_t get_dfar();

void do_reset();
void do_undefined_instruction();
void do_supervisor_call();
void do_prefetch_abort();
void do_data_abort();
void do_irq_interrupt();
void do_fiq_interrupt();

#endif
