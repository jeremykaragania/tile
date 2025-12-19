#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <kernel/list.h>

struct buffer_info {
  uint32_t num;
  int status;
  char* data;
  struct list_link link;
};

/*
  "buffers_head" is the head node of the buffer information list.
*/
extern struct list_link buffers_head;

struct buffer_info* buffer_get(uint32_t num);
void buffer_put(struct buffer_info* buffer_info);
void buffer_write(struct buffer_info* buffer_info);

#endif
