#include <kernel/buffer.h>

char** buffer_cache;
struct buffer_info* buffer_info_cache;
struct buffer_info free_buffer_infos;

void buffer_init() {
  struct buffer_info* curr;

  /*
    Initialize the buffer cache.
  */
  buffer_cache = memory_alloc(sizeof(char*) * BUFFER_INFO_CACHE_SIZE, 1);

  for (size_t i = 0; i < BUFFER_INFO_CACHE_SIZE; ++i) {
    buffer_cache[i] = memory_alloc(FILE_BLOCK_SIZE, 1);
  }

  /*
    Initialize the buffer information cache and the free buffer information list.
  */
  buffer_info_cache = memory_alloc(sizeof(struct buffer_info) * BUFFER_INFO_CACHE_SIZE, 1);

  free_buffer_infos.next = &buffer_info_cache[0];
  free_buffer_infos.prev = &buffer_info_cache[BUFFER_INFO_CACHE_SIZE - 1];

  curr = free_buffer_infos.next;

  for (size_t i = 0; i < BUFFER_INFO_CACHE_SIZE; ++i) {
    if (i == BUFFER_INFO_CACHE_SIZE - 1) {
      curr->next = &free_buffer_infos;
    }
    else {
      curr->next = &buffer_info_cache[i + 1];
    }

    curr->prev = &buffer_info_cache[i - 1];
    curr = curr->next;
  }
}
