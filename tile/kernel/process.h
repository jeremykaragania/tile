#ifndef PROCESS_H
#define PROCESS_H

enum process_state  {
  CREATED,
  READY,
  RUNNING,
  BLOCKED,
  TERMINATED
};

struct process_table_entry {
  int id;
  int state;
  void* stack;
};

#endif
