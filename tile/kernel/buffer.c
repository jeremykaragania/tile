#include <kernel/buffer.h>

/*
  "buffer_pool" is a pool for buffers. It is the memory backing all buffer
  data.
*/
static char buffer_pool[BUFFER_INFO_POOL_SIZE * BLOCK_SIZE];

/*
  "buffer_info_pool" is a pool for buffer information. It is the memory
  backing all buffer information.
*/
static struct buffer_info buffer_info_pool[BUFFER_INFO_POOL_SIZE];

struct buffer_info buffer_infos;
struct buffer_info free_buffer_infos;

void buffer_init() {
  struct buffer_info* curr;

  /*
    Initialize the free buffer information list.
  */
  buffer_infos.next = &buffer_infos;
  buffer_infos.prev = &buffer_infos;

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

    curr->data = &buffer_pool[i * BLOCK_SIZE];
    curr->prev = &buffer_info_pool[i - 1];
    curr = curr->next;
  }
}

/*
  buffer_get reads the filesystem for the block number "num" and returns buffer
  information for it.
*/
struct buffer_info* buffer_get(uint32_t num) {
  struct buffer_info* curr = buffer_infos.next;

  /*
    We first check if the buffer information is in the cache. If it is, then we
    return it.
  */
  while (curr != &buffer_infos) {
    if (curr->num == num) {
      return curr;
    }

    curr = curr->next;
  }

  /*
    If the buffer information isn't in the cache, then we allocate it.
  */
  curr = buffer_pop(&free_buffer_infos);

  /*
    If there are no free buffers, then we wait.
  */
  while (!curr) {
    curr = buffer_pop(&free_buffer_infos);
  }

  curr->num = num;

  for (size_t i = 0; i < BLOCK_SIZE / MCI_BLOCK_SIZE; ++i) {
    size_t offset = i * MCI_BLOCK_SIZE;

    mci_read(BLOCK_SIZE * num + offset, curr->data + offset);
  }

  buffer_push(&buffer_infos, curr);
  return curr;
}

/*
  buffer_put writes the buffer information "buffer_info" to the filesystem and
  frees it.
*/
void buffer_put(struct buffer_info* buffer_info) {
  buffer_write(buffer_info);
  buffer_remove(&buffer_infos, buffer_info);
  buffer_push(&free_buffer_infos, buffer_info);
}

/*
  buffer_write writes the buffer information "buffer_info" to the filesystem.
*/
void buffer_write(struct buffer_info* buffer_info) {
  for (size_t i = 0; i < BLOCK_SIZE / MCI_BLOCK_SIZE; ++i) {
    size_t offset = i * MCI_BLOCK_SIZE;

    mci_write(BLOCK_SIZE * buffer_info->num + offset, buffer_info->data + offset);
  }
}

/*
  buffer_push adds an element "buffer_info" to the buffer information list
  "list".
*/
void buffer_push(struct buffer_info* list, struct buffer_info* buffer_info) {
  buffer_info->next = list->next;
  buffer_info->prev = list;
  list->next->prev = buffer_info;
  list->next = buffer_info;
}

/*
  buffer_pop removes the first element of the buffer information list "list"
  and returns it.
*/
struct buffer_info* buffer_pop(struct buffer_info* list) {
  struct buffer_info* ret = list->next;

  if (ret == list) {
    return NULL;
  }

  list->next = ret->next;
  ret->next->prev = list;
  return ret;
}

/*
  buffer_remove removes the buffer information "buffer_info" from the
  buffer information list "list".
*/
void buffer_remove(struct buffer_info* list, struct buffer_info* buffer_info) {
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
