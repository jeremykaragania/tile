#ifndef TERMINAL_H
#define TERMINAL_H

#include <kernel/list.h>
#include <kernel/fifo.h>
#include <kernel/file.h>

#define TERMINAL_CHAR_ERASE  0x7f
#define TERMINAL_CHAR_NL 0x0a
#define TERMINAL_CHAR_CR 0x0d

#define TERMINAL_FIFO_SIZE 256
#define LINE_BUFFER_SIZE 256

/*
  struct line_buffer represents a line buffer to store processed or "cooked"
  input.
*/
struct line_buffer {
  char buf[LINE_BUFFER_SIZE];
  size_t cursor;
};

/*
  struct terminal represents a terminal device. A terminal device manages a
  low-level serial driver's operations.
*/
struct terminal {
  struct uart_operations* ops;
  struct fifo raw;
  struct line_buffer cooked;
  void* private;
};

extern struct file_operations terminal_operations;

struct terminal* terminal_alloc();
void terminal_free(struct terminal* term);

int terminal_read(struct file_info_int* file, void* buf, size_t count);
int terminal_write(struct file_info_int* file, const void* buf, size_t count);

int terminal_process_input_char(struct terminal* term, char c);
size_t terminal_process_output_block(struct terminal* term, const void* buf, size_t count);
void terminal_process_output_char(struct terminal* term, char c);

void terminal_echo_char(struct terminal* term, char c);

struct terminal* file_to_terminal(struct file_info_int* file);

int line_buffer_insert_char(struct line_buffer* lb, int c);
int line_buffer_insert(struct line_buffer* lb, char* s, size_t count);
int line_buffer_remove_char(struct line_buffer* lb);
int line_buffer_remove(struct line_buffer* lb, size_t count);

#endif
