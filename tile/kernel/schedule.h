#ifndef SCHEDULE_H
#define SCHEDULE_C

#include <kernel/process.h>

#define SCHEDULE_QUEUE_SIZE 32

void schedule_init();

extern struct process_info* schedule_queue[SCHEDULE_QUEUE_SIZE];

#endif
