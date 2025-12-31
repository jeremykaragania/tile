#include <drivers/terminal.h>
#include <drivers/pl011.h>
#include <kernel/device.h>
#include <kernel/memory.h>

struct file_operations terminal_operations = {
  .write = terminal_write
};

/*
  terminal_alloc allocates and returns a terminal structure and certain fields
  within it.
*/
struct terminal* terminal_alloc() {
  struct terminal* term;

  term = memory_alloc(sizeof(struct terminal));
  fifo_alloc(&term->fifo_raw, TERMINAL_FIFO_SIZE, 1);
  fifo_alloc(&term->fifo_cooked, TERMINAL_FIFO_SIZE, 1);

  return term;
}

/*
  terminal_free frees the terminal "term" and any allocated fields.
*/
void terminal_free(struct terminal* term) {
  fifo_free(&term->fifo_raw);
  fifo_free(&term->fifo_cooked);
  memory_free(term);
}

/*
  terminal_write writes up to "count" bytes from the buffer "buf" to the
  terminal.
*/
int terminal_write(struct file_info_int* file, const void* buf, size_t count) {
  struct terminal* term;

  term = file_to_terminal(file);

  for (size_t i = 0; i < count; ++i) {
    terminal_process_output_char(term, ((char*)buf)[i]);
  }

  return count;
}

/*
  terminal_process_output_char processes a character "c" and writes it to the
  underlying device.
*/
void terminal_process_output_char(struct terminal* term, const char c) {
  struct uart* u;

  u = term->private;

  switch (c) {
    case '\n':
      term->ops->write(u, "\r\n", 2);
      break;
    case '\t':
      term->ops->write(u, "       ", 8);
      break;
    default:
      term->ops->write(u, &c, 1);
      break;
  }
}

/*
  file_to_terminal tries to return the private terminal information of a device
  from the file "file".
*/
struct terminal* file_to_terminal(struct file_info_int* file) {
  struct device* dev;

  dev = file_to_device(file);

  if (!dev) {
    return NULL;
  }

  return (struct terminal*)dev->private;
}
