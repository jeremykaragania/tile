#ifndef BUFFER_H
#define BUFFER_H

#include <drivers/pl180.h>
#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <stdint.h>

#define BUFFER_INFO_POOL_SIZE 32

/*
  "buffer_pool" is a pool for buffers. It is the memory backing all buffer
  data.
*/
extern char** buffer_pool;

/*
  "buffer_info_pool" is a pool for buffer information. It is the memory
  backing all buffer information.
*/
extern struct buffer_info* buffer_info_pool;

/*
  "buffer_info_cache" is a cache for buffer information.
*/
extern struct buffer_info buffer_info_cache;

/*
  "free_buffer_infos" is a doubly linked list which stores the buffer
  information in "buffer_info_pool" which are not being used.
*/
extern struct buffer_info free_buffer_infos;

struct buffer_info {
  uint32_t num;
  char* data;
  struct buffer_info* next;
  struct buffer_info* prev;
};

void buffer_init();

struct buffer_info* buffer_info_get(uint32_t num);

void buffer_info_push(struct buffer_info* list, struct buffer_info* buffer_info);
struct buffer_info* buffer_info_pop(struct buffer_info* list);

#endif
