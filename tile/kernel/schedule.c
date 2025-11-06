#include <kernel/schedule.h>
#include <kernel/process.h>
#include <kernel/list.h>
#include <kernel/page.h>

/*
  schedule_init initializes the scheduler.
*/
void schedule_init() {
  list_init(&processes_head);
  list_push(&processes_head, &init_process.link);
}

/*
  schedule_tick determines if the current process needs to be rescheduled.
*/
void schedule_tick() {
  /* For now, the current process is always rescheduled. */
  current->sched.reschedule = 1;
}

/*
  schedule schedules the next process to be executed and context switches to
  it.
*/
void schedule() {
  struct list_link* next;
  struct process_info* proc;

  if (!current->sched.reschedule) {
    return;
  }

  next = current->link.next;

  if (next == &processes_head) {
    next = next->next;
  }

  proc = list_data(next, struct process_info, link);

  /* If the next process has a different memory context, then we switch it too. */
  if (current->mem != proc->mem) {
    set_pgd(proc->mem->pgd);
  }

  context_switch(&current->context_reg, &proc->context_reg);
}

void enable_preemption() {
  current->sched.preempt = 1;
}

void disable_preemption() {
  current->sched.preempt = 0;
}
