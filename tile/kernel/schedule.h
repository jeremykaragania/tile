#ifndef SCHEDULE_H
#define SCHEDULE_C

#include <kernel/process.h>

void schedule_init();
void schedule_tick();
void schedule();

extern void context_switch(struct processor_registers* from, struct processor_registers *to);

#endif
