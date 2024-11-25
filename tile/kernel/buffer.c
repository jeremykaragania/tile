#include <kernel/buffer.h>

char** buffer_pool;
struct buffer_info* buffer_info_pool;
struct buffer_info buffer_info_cache;
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

  buffer_info_cache.next = &buffer_info_cache;
  buffer_info_cache.prev = &buffer_info_cache;

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

    curr->data = buffer_pool[i];
    curr->prev = &buffer_info_pool[i - 1];
    curr = curr->next;
  }
}

/*
  buffer_info_get reads the filesystem for the block number "num" and returns
  buffer information for it.
*/
struct buffer_info* buffer_info_get(uint32_t num) {
  struct buffer_info* curr = buffer_info_cache.next;

  /*
    We first check if the buffer information is in the cache. If it is, then we
    return it.
  */
  while (curr != &buffer_info_cache) {
    if (curr->num == num) {
      return curr;
    }

    curr = curr->next;
  }

  /*
    If the buffer information isn't in the cache, then we allocate it.
  */
  curr = buffer_info_pop(&free_buffer_infos);

  if (!curr) {
    return NULL;
  }

  curr->num = num;
  mci_read(FILE_BLOCK_SIZE * num, curr->data);
  buffer_info_push(&buffer_info_cache, curr);
  return curr;
}

/*
  buffer_info_put writes the buffer information "buffer_info" to the filesystem
  and frees it.
*/
void buffer_info_put(struct buffer_info* buffer_info) {
  mci_write(FILE_BLOCK_SIZE * buffer_info->num, buffer_info->data);
  buffer_info_remove(&buffer_info_cache, buffer_info);
  buffer_info_push(&free_buffer_infos, buffer_info);
}

/*
  buffer_info_push adds an element "buffer_info" to the buffer information list
  "list".
*/
void buffer_info_push(struct buffer_info* list, struct buffer_info* buffer_info) {
  buffer_info->next = list->next;
  buffer_info->prev = list;
  list->next->prev = buffer_info;
  list->next = buffer_info;
}

/*
  buffer_info_pop removes the first element of the buffer information list
  "list" and returns it.
*/
struct buffer_info* buffer_info_pop(struct buffer_info* list) {
  struct buffer_info* ret = list->next;

  if (ret == &free_buffer_infos) {
    return NULL;
  }

  list->next = ret->next;
  ret->next->prev = list;
  return ret;
}

/*
  buffer_info_remove removes the buffer information "buffer_info" from the
  buffer information list "list".
*/
void buffer_info_remove(struct buffer_info* list, struct buffer_info* buffer_info) {
  /*
    Not allowed to remove the head buffer information element or remove from an
    empty list.
  */
  if (list == buffer_info || list->next == list) {
    return;
  }

  buffer_info->next->prev = buffer_info->prev;
  buffer_info->prev->next = buffer_info->next;
}
