#include <kernel/buffer.h>

char** buffer_pool;
struct buffer_info* buffer_info_pool;
struct buffer_info free_buffer_infos;

void buffer_init() {
  struct buffer_info* curr;

  /*
    Initialize the buffer pool.
  */
  buffer_pool = memory_alloc(sizeof(char*) * BUFFER_INFO_POOL_SIZE, 1);

  for (size_t i = 0; i < BUFFER_INFO_POOL_SIZE; ++i) {
    buffer_pool[i] = memory_alloc(FILE_BLOCK_SIZE, 1);
  }

  /*
    Initialize the buffer information pool and the free buffer information list.
  */
  buffer_info_pool = memory_alloc(sizeof(struct buffer_info) * BUFFER_INFO_POOL_SIZE, 1);

  free_buffer_infos.next = &buffer_info_pool[0];
  free_buffer_infos.prev = &buffer_info_pool[BUFFER_INFO_POOL_SIZE - 1];

  curr = free_buffer_infos.next;

  for (size_t i = 0; i < BUFFER_INFO_POOL_SIZE; ++i) {
    if (i == BUFFER_INFO_POOL_SIZE - 1) {
      curr->next = &free_buffer_infos;
    }
    else {
      curr->next = &buffer_info_pool[i + 1];
    }

    curr->prev = &buffer_info_pool[i - 1];
    curr = curr->next;
  }
}

/*
  free_buffer_infos_push adds an element "buffer_info" to the free buffer
  information list.
*/
void free_buffer_infos_push(struct buffer_info* buffer_info) {
  buffer_info->next = free_buffer_infos.next;
  buffer_info->prev = &free_buffer_infos;
  free_buffer_infos.next->prev = buffer_info;
  free_buffer_infos.next = buffer_info;
}

/*
  free_buffer_infos_pop removes the first element of the free buffer
  information list and returns it.
*/
struct buffer_info* free_buffer_infos_pop() {
  struct buffer_info* ret = free_buffer_infos.next;

  if (ret == &free_buffer_infos) {
    return NULL;
  }

  free_buffer_infos.next = ret->next;
  ret->next->prev = &free_buffer_infos;
  return ret;
}
