#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <kernel/processor.h>

#define TIM01INT 34

extern uint32_t get_dfar();

void do_reset();
void do_undefined_instruction();
void do_supervisor_call();
void do_prefetch_abort();
void do_data_abort();
void do_irq_interrupt();
void do_fiq_interrupt();

void ret_from_interrupt(struct processor_registers* registers);
void ret_from_interrupt_user();

#endif
