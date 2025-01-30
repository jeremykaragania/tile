#ifndef BUFFER_H
#define BUFFER_H

#include <drivers/pl180.h>
#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <stdint.h>

#define BUFFER_INFO_POOL_SIZE 32

struct buffer_info {
  uint32_t num;
  char* data;
  struct buffer_info* next;
  struct buffer_info* prev;
};

/*
  "buffer_infos" is a cache for buffer information.
*/
extern struct buffer_info buffer_infos;

/*
  "free_buffer_infos" is a doubly linked list which stores the buffer
  information in "buffer_info_pool" which are not being used.
*/
extern struct buffer_info free_buffer_infos;

void buffer_init();

struct buffer_info* buffer_get(uint32_t num);
void buffer_put(struct buffer_info* buffer_info);

void buffer_push(struct buffer_info* list, struct buffer_info* buffer_info);
struct buffer_info* buffer_pop(struct buffer_info* list);
void buffer_remove(struct buffer_info* list, struct buffer_info* buffer_info);

#endif
