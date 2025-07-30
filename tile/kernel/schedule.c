#include <kernel/schedule.h>
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
  schedule schedules the next process to be executed and context switches to
  it.
*/
void schedule() {
  struct list_link* next;
  struct process_info* proc;

  next = current->link.next;

  if (next == &processes_head) {
    next = next->next;
  }

  proc = list_data(next, struct process_info, link);

  if (current->pgd != proc->pgd) {
    set_pgd(proc->pgd);
  }

  context_switch(&current->reg, &proc->reg);
}
