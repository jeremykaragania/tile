#include <kernel/schedule.h>

struct process_info* schedule_queue[SCHEDULE_QUEUE_SIZE];

/*
  schedule_init initializes the scheduler.
*/
void schedule_init() {
  schedule_queue[0] = &init_process;
}
