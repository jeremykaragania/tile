#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>
#include <kernel/processor.h>

#define TIM01INT 34


int handle_fault(uint32_t addr);

void do_reset();
void do_undefined_instruction();
void do_supervisor_call();
void do_prefetch_abort();
void do_data_abort();
void do_irq_interrupt();
void do_fiq_interrupt();

extern uint32_t get_dfar();
extern void ret_from_interrupt(struct processor_registers* registers);
extern void ret_from_interrupt_user();

#endif
