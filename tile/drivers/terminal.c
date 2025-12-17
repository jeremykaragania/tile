#include <drivers/terminal.h>
#include <drivers/pl011.h>
#include <kernel/device.h>

struct file_operations terminal_operations = {
  .write = terminal_write
};

struct terminal terminal = {
  .ops = &uart_operations
};

struct device terminal_device = {
  .name = "console",
  .ops = &terminal_operations,
  .major = 5,
  .minor = 1,
  .type = DT_CHARACTER,
  .private = &terminal
};

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

/*
  terminal_write writes up to "count" bytes from the buffer "buf" to the
  terminal.
*/
int terminal_write(int fd, const void* buf, size_t count) {
  struct terminal* term;
  size_t n;
  size_t remaining;

  remaining = count;
  term = file_to_terminal(fd_to_file(fd));

  while (remaining) {
    n = term->ops->write(fd, buf, remaining);
    remaining -= n;
    buf = (char*)buf + n;
  }

  return count;
}
