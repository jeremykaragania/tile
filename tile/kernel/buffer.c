#include <kernel/buffer.h>
#include <drivers/pl180.h>
#include <kernel/asm/file.h>
#include <kernel/memory.h>

struct list_link buffers_head;

void buffer_init() {
  list_init(&buffers_head);
}

/*
  buffer_get reads the filesystem for the block number "num" and returns buffer
  information for it.
*/
struct buffer_info* buffer_get(uint32_t num) {
  struct buffer_info* buffer;
  struct list_link* curr;

  curr = buffers_head.next;

  /*
    We first check if the buffer information is in the cache. If it is, then we
    return it.
  */
  while (curr != &buffers_head) {
    buffer = list_data(curr, struct buffer_info, link);

    if (buffer->num == num) {
      return buffer;
    }

    curr = curr->next;
  }

  /*
    If the buffer information isn't in the cache, then we allocate it.
  */
  buffer = memory_alloc(sizeof(struct buffer_info));
  buffer->data = memory_alloc(BLOCK_SIZE);

  buffer->num = num;

  for (size_t i = 0; i < BLOCK_SIZE / MCI_BLOCK_SIZE; ++i) {
    size_t offset = i * MCI_BLOCK_SIZE;

    mci_read(BLOCK_SIZE * num + offset, buffer->data + offset);
  }

  list_push(&buffers_head, &buffer->link);
  return buffer;
}

/*
  buffer_put writes the buffer information "buffer_info" to the filesystem and
  frees it.
*/
void buffer_put(struct buffer_info* buffer_info) {
  buffer_write(buffer_info);
  list_remove(&buffers_head, &buffer_info->link);
  memory_free(buffer_info->data);
  memory_free(buffer_info);
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
