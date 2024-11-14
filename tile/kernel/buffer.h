#ifndef BUFFER_H
#define BUFFER_H

#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <stdint.h>

#define BUFFER_INFO_CACHE_SIZE 32

/*
  "buffer_cache" is a cache for buffers. It is the memory backing all buffer
  data.
*/
extern char** buffer_cache;

/*
  "buffer_info_cache" is a cache for buffer information. It is the memory
  backing all buffer information.
*/
extern struct buffer_info* buffer_info_cache;

/*
  "free_buffer_infos" is a doubly linked list which stores the buffer
  information in "buffer_info_cache" which are not being used.
*/
extern struct buffer_info free_buffer_infos;

struct buffer_info {
  uint32_t num;
  char* data;
  struct buffer_info* next;
  struct buffer_info* prev;
};

void buffer_init();

#endif
