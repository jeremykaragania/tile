#ifndef SCHEDULE_H
#define SCHEDULE_C

#include <kernel/process.h>

#define SCHEDULE_QUEUE_SIZE 32

void schedule_init();
void schedule();

extern struct process_info* schedule_queue[SCHEDULE_QUEUE_SIZE];
extern void context_switch(struct processor_registers* from, struct processor_registers *to);

#endif
