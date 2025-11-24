#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <kernel/processor.h>
#include <stdbool.h>

/*
  schedule_info represents scheduling information about a process.
*/
struct schedule_info {
  bool reschedule;
  bool preempt;
};

void schedule_init();
void schedule_tick();
void schedule();

void enable_preemption();
void disable_preemption();

extern void context_switch(struct processor_registers* from, struct processor_registers *to);

#endif
