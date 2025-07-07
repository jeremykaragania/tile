#include <kernel/schedule.h>

struct process_info* schedule_queue[SCHEDULE_QUEUE_SIZE];

/*
  schedule_init initializes the scheduler.
*/
void schedule_init() {
  schedule_queue[0] = &init_process;
}

/*
  schedule schedules the next process to be executed and context switches to
  it.
*/
void schedule() {
  struct process_info* curr = current;
  struct process_info* next = current->next;

  if (next == &process_infos) {
    next = next->next;
  }

  context_switch(&current->reg, &next->reg);
}
