#ifndef TERMINAL_H
#define TERMINAL_H

#include <kernel/list.h>
#include <kernel/file.h>

/*
  struct terminal represents a terminal device. A terminal device manages a
  low-level serial driver's operations.
*/
struct terminal {
  struct file_operations* ops;
};

extern struct file_operations terminal_operations;
extern struct terminal terminal;
extern struct device terminal_device;

struct terminal* file_to_terminal(struct file_info_int* file);

int terminal_write(int fd, const void* buf, size_t count);

#endif
