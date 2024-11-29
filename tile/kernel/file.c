#include <kernel/file.h>

struct filesystem_info filesystem_info;
struct file_info_int* file_info_pool;
struct file_info_int file_info_cache;
struct file_info_int free_file_infos;

/*
  filesystem_init initializes the filesystem and the relevant structures used
  by the kernel. It assumes that the filesystem begins at the beginning address
  of the SD card.
*/
void filesystem_init() {
  struct buffer_info* buffer_info = NULL;
  struct file_info_int* curr;

  /*
    The first block contains information about the filesystem. The memory
    layout is specified by struct filesystem_info.
  */
  buffer_info = buffer_info_get(0);
  filesystem_info = *(struct filesystem_info*)buffer_info->data;

  /*
    Initialize the file information pool and the free file information list.
  */
  file_info_pool = memory_alloc(sizeof(struct file_info_int) * FILE_INFO_CACHE_SIZE, 1);

  file_info_cache.next = &file_info_cache;
  file_info_cache.prev = &file_info_cache;

  free_file_infos.next = &file_info_pool[0];
  free_file_infos.prev = &file_info_pool[FILE_INFO_CACHE_SIZE - 1];

  curr = free_file_infos.next;

  for (size_t i = 0; i < FILE_INFO_CACHE_SIZE; ++i) {
    if (i == FILE_INFO_CACHE_SIZE - 1) {
      curr->next = &free_file_infos;
    }
    else {
      curr->next = &file_info_pool[i + 1];
    }

    curr->prev = &file_info_pool[i - 1];
    curr = curr->next;
  }
}

/*
  file_info_int_get returns internal file information from an external file
  information number "file_info_num".
*/
struct file_info_int* file_info_get(uint32_t file_info_num) {
  struct file_info_int* ret = NULL;
  struct buffer_info* buffer_info = NULL;
  uint32_t block_num = 1 + (file_info_num - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_ext) * ((file_info_num - 1) % FILE_INFO_PER_BLOCK);

  buffer_info = buffer_info_get(block_num);
  ret = file_info_pop(&free_file_infos);

  if (!buffer_info || !ret) {
    return NULL;
  }

  ret->ext = *(struct file_info_ext*)(buffer_info->data + block_offset);
  return ret;
}

/*
  file_info_int_put writes the external file information from the internal file
  information "file_info" to the SD card.
*/
void file_info_put(const struct file_info_int* file_info) {
  struct buffer_info* buffer_info = NULL;
  uint32_t block_num = 1 + (file_info->ext.num - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_ext) * ((file_info->ext.num - 1) % FILE_INFO_PER_BLOCK);

  buffer_info = buffer_info_get(block_num);
  *(struct file_info_ext*)(buffer_info->data + block_offset) = file_info->ext;
  buffer_info_put(buffer_info);
}

/*
  file_info_push adds an element "file_info" to the free file information list
  "list".
*/
void file_info_push(struct file_info_int* list, struct file_info_int* file_info) {
  file_info->next = list->next;
  file_info->prev = list;
  list->next->prev = file_info;
  list->next = file_info;
}

/*
  file_info_pop removes the first element of the free file information list
  "list" and returns it.
*/
struct file_info_int* file_info_pop(struct file_info_int* list) {
  struct file_info_int* ret = list->next;

  if (ret == list) {
    return NULL;
  }

  list->next = ret->next;
  ret->next->prev = list;
  return ret;
}

/*
  block_alloc allocates a block using the free block list and returns it.
*/
struct buffer_info* block_alloc() {
  uint32_t num;
  struct buffer_info* ret;

  --filesystem_info.free_blocks_size;
  num = filesystem_info.free_blocks_cache[filesystem_info.free_blocks_size];
  ret = buffer_info_get(num);

  /*
    If we just read the last block from the free block list, then we replenish
    the free block list.
  */
  if (!filesystem_info.free_blocks_size) {
    filesystem_info.free_blocks_size = *((uint32_t*)ret->data);
    memcpy(filesystem_info.free_blocks_cache, (uint32_t*)ret->data + 1, filesystem_info.free_blocks_size * sizeof(uint32_t));
  }

  memset(ret->data, 0, FILE_BLOCK_SIZE);
  return ret;
}

/*
  block_free frees the block specified by "buffer_info". It tries to push the
  block to the free block list.
*/
void block_free(struct buffer_info* buffer_info) {
  /*
    If the free block list is full and we can't push to it, then we write it to
    disk and point to it.
  */
  if (filesystem_info.free_blocks_size + 1 > FILESYSTEM_INFO_CACHE_SIZE) {
    *((uint32_t*)buffer_info->data) = filesystem_info.free_blocks_size;
    memcpy((uint32_t*)buffer_info->data + 1, filesystem_info.free_blocks_cache, filesystem_info.free_blocks_size * sizeof(uint32_t));
    filesystem_info.free_blocks_size = 1;
    *filesystem_info.free_blocks_cache = buffer_info->num;
    buffer_info_put(buffer_info);
  }
  else {
    filesystem_info.free_blocks_cache[filesystem_info.free_blocks_size] = buffer_info->num;
    ++filesystem_info.free_blocks_size;
  }
}
