#include <kernel/fifo.h>
#include <kernel/memory.h>
#include <lib/string.h>

/*
  fifo_alloc allocates a FIFO buffer of "n" elements of size "size" in the FIFO
  "fifo" and initializes it.
*/
int fifo_alloc(struct fifo* fifo, size_t n, size_t size) {
  fifo->buf = memory_alloc(n * size);

  if (!fifo->buf) {
    return -1;
  }

  fifo->begin = 0;
  fifo->end = 0;
  fifo->n = n;
  fifo->size = size;

  return 0;
}

/*
  fifo_free frees the FIFO's buffer in the FIFO "fifo".
*/
void fifo_free(struct fifo* fifo) {
  memory_free(fifo->buf);
}

/*
  fifo_push pushes the element "src" to the FIFO "fifo".
*/
int fifo_push(struct fifo* fifo, void* src) {
  if (is_fifo_full(fifo)) {
    return -1;
  }

  memcpy((char*)fifo->buf + (fifo->end * fifo->size), src, fifo->size);
  fifo->end = (fifo->end + 1) % fifo->n;

  return 0;
}

/*
  fifo_pop pops the first element from the FIFO "fifo" into "dest".
*/
int fifo_pop(struct fifo* fifo, void* dest) {
  if (is_fifo_empty(fifo)) {
    return -1;
  }

  memcpy(dest, (char*)fifo->buf + (fifo->begin * fifo->size), fifo->size);
  fifo->begin = (fifo->begin + 1) % fifo->n;

  return 0;
}

/*
  is_fifo_full returns if the FIFO "fifo" is full.
*/
bool is_fifo_full(struct fifo* fifo) {
  if ((fifo->end + 1) % fifo->n == fifo->begin) {
    return true;
  }

  return false;
}

/*
  is_fifo_empty returns if the FIFO "fifo" is empty.
*/
bool is_fifo_empty(struct fifo* fifo) {
  if (fifo->begin == fifo->end) {
    return true;
  }

  return false;
}
