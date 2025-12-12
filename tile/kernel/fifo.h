#ifndef FIFO_H
#define FIFO_H

#include <stddef.h>
#include <stdbool.h>

struct fifo {
  void* buf;
  size_t begin;
  size_t end;
  size_t n;
  size_t size;
};

int fifo_alloc(struct fifo* fifo, size_t n, size_t size);
void fifo_free(struct fifo* fifo);

int fifo_push(struct fifo* fifo, const void* src);
int fifo_pop(struct fifo* fifo, void* dest);

size_t fifo_push_n(struct fifo* fifo, const void* src, size_t n);
size_t fifo_pop_n(struct fifo* fifo, void* dest, size_t n);

bool is_fifo_full(struct fifo* fifo);
bool is_fifo_empty(struct fifo* fifo);

#endif
