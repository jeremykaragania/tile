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
  fifo_alloc(&term->fifo, TERMINAL_FIFO_SIZE, 1);

  return term;
}

/*
  terminal_free frees the terminal "term" and any allocated fields.
*/
void terminal_free(struct terminal* term) {
  fifo_free(&term->fifo);
  memory_free(term);
}

/*
  terminal_write writes up to "count" bytes from the buffer "buf" to the
  terminal.
*/
int terminal_write(struct file_info_int* file, const void* buf, size_t count) {
  struct terminal* term;
  size_t n;
  size_t remaining;

  remaining = count;
  term = file_to_terminal(file);

  while (remaining) {
    n = term->ops->write(file, buf, remaining);
    remaining -= n;
    buf = (char*)buf + n;
  }

  return count;
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
