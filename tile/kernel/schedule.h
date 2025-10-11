#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <kernel/processor.h>

/*
  schedule_info represents scheduling information about a process.
*/
struct schedule_info {
  int reschedule;
};

void schedule_init();
void schedule_tick();
void schedule();

extern void context_switch(struct processor_registers* from, struct processor_registers *to);

#endif
