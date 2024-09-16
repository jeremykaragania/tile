#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

#define BUFFER_CACHE_SIZE 32

extern char* buffer_cache;
extern struct buffer_info free_buffer_infos;

struct buffer_info {
  uint32_t num;
  char* data;
  struct buffer_int* next;
  struct buffer_int* prev;
};

#endif
