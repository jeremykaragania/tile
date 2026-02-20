#include <drivers/terminal.h>
#include <drivers/pl011.h>
#include <kernel/device.h>
#include <kernel/memory.h>
#include <lib/string.h>
#include <stdbool.h>

struct file_operations terminal_operations = {
  .read = terminal_read,
  .write = terminal_write
};

/*
  terminal_alloc allocates and returns a terminal structure and certain fields
  within it.
*/
struct terminal* terminal_alloc() {
  struct terminal* term;

  term = memory_alloc(sizeof(struct terminal));
  fifo_alloc(&term->raw, TERMINAL_FIFO_SIZE, 1);
  term->cooked.cursor = 0;

  return term;
}

/*
  terminal_free frees the terminal "term" and any allocated fields.
*/
void terminal_free(struct terminal* term) {
  fifo_free(&term->raw);
  memory_free(term);
}

/*
  terminal_read reads up to "count" bytes into the buffer "buf".
*/
int terminal_read(struct file_info_int* file, void* buf, size_t count) {
  struct terminal* term;
  struct line_buffer* lb;
  char c;
  bool is_done;
  size_t insert_count;

  term = file_to_terminal(file);
  lb = &term->cooked;
  is_done = false;
  insert_count = 0;

  while (1) {
    while (fifo_pop(&term->raw, &c) < 0);

    switch (c) {
      case TERMINAL_CHAR_ERASE:
        line_buffer_remove_char(lb);
        break;
      case TERMINAL_CHAR_CR:
        insert_count = line_buffer_insert_char(lb, '\n');
        is_done = true;
        break;
      default:
        insert_count = line_buffer_insert_char(lb, c);
        break;
    }

    if (is_done || insert_count == 0) {
      break;
    }
  }

  if (lb->cursor < count) {
    count = lb->cursor;
  }

  memcpy(buf, lb->buf, count);

  lb->cursor = 0;

  return count;
}

/*
  terminal_write writes up to "count" bytes from the buffer "buf" to the
  terminal.
*/
int terminal_write(struct file_info_int* file, const void* buf, size_t count) {
  const char* b;
  size_t c;
  struct terminal* term;

  b = buf;
  term = file_to_terminal(file);

  while (count > 0) {
    c = terminal_process_output_block(term, b, count);
    b += c;
    count -= c;

    if (count == 0) {
      break;
    }

    terminal_process_output_char(term, *b);
    ++b;
    --count;
  }

  uart_begin(term->private);

  return count;
}

/*
  terminal_process_input_char processes the input character "c".
*/
int terminal_process_input_char(struct terminal* term, char c) {
  terminal_echo_char(term, c);

  return fifo_push(&term->raw, &c);
}

/*
  terminal_process_output_block will try to output as many non-special
  characters from the buffer "buf" and return the number of characters written.
*/
size_t terminal_process_output_block(struct terminal* term, const void* buf, size_t count) {
  bool do_break;
  size_t i;
  struct uart* u;

  do_break = false;
  u = term->private;

  for (i = 0; i < count; ++i) {
    char c = ((char*)buf)[i];

    switch (c) {
      case '\n':
      case '\t':
        do_break = true;
        break;
      default:
        break;
    }

    if (do_break) {
      break;
    }
  }

  return term->ops->write(u, buf, i);
}

/*
  terminal_process_output_char processes a character "c" and writes it to the
  underlying device.
*/
void terminal_process_output_char(struct terminal* term, char c) {
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
  terminal_echo_char echoes the character "c" to the terminal "term".
*/
void terminal_echo_char(struct terminal* term, char c) {
  struct uart* u;

  u = term->private;

  switch (c) {
    case TERMINAL_CHAR_ERASE:
      term->ops->write(u, "\b \b", 3);
      break;
    case TERMINAL_CHAR_CR:
      term->ops->write(u, "\r\n", 2);
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

/*
  line_buffer_insert_char inserts the character "c" at the end of the line
  buffer "lb".
*/
int line_buffer_insert_char(struct line_buffer* lb, int c) {
  if (lb->cursor == LINE_BUFFER_SIZE - 1) {
    return -1;
  }

  lb->buf[lb->cursor++] = c;

  return c;
}

/*
  line_buffer_insert inserts "count" characters from the string "s" into the
  line buffer "lb".
*/
int line_buffer_insert(struct line_buffer* lb, char* s, size_t count) {
  size_t i;

  i = 0;

  while (i < count) {
    if (line_buffer_insert_char(lb, s[i]) < 0) {
      break;
    }

    ++i;
  }

  return i;
}

/*
  line_buffer_remove_char removes the last character from the line buffer "lb".
*/
int line_buffer_remove_char(struct line_buffer* lb) {
  if (lb->cursor == 0) {
    return -1;
  }

  --lb->cursor;

  return 0;
}

/*
   line_buffer_remove removes "count" characters from the end of the line
   buffer "lb".
*/
int line_buffer_remove(struct line_buffer* lb, size_t count) {
  size_t i;

  i = 0;

  while (i < count) {
    if (line_buffer_remove_char(lb) < 0) {
      break;
    }

    ++i;
  }

  return i;
}
