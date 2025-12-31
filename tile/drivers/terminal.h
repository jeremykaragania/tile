#ifndef TERMINAL_H
#define TERMINAL_H

#include <kernel/list.h>
#include <kernel/fifo.h>
#include <kernel/file.h>

#define TERMINAL_FIFO_SIZE 256

/*
  struct terminal represents a terminal device. A terminal device manages a
  low-level serial driver's operations.
*/
struct terminal {
  struct uart_operations* ops;
  struct fifo fifo_raw;
  struct fifo fifo_cooked;
  void* private;
};

extern struct file_operations terminal_operations;

struct terminal* terminal_alloc();
void terminal_free(struct terminal* term);

int terminal_write(struct file_info_int* file, const void* buf, size_t count);

void terminal_process_output_char(struct terminal* term, const char c);

struct terminal* file_to_terminal(struct file_info_int* file);

#endif
