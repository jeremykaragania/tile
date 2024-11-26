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
  ret = free_file_infos_pop();

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
  free_file_infos_push adds an element "file_info" to the free file information
  list.
*/
void free_file_infos_push(struct file_info_int* file_info) {
  file_info->next = free_file_infos.next;
  file_info->prev = &free_file_infos;
  free_file_infos.next->prev = file_info;
  free_file_infos.next = file_info;
}

/*
  free_file_infos_pop removes the first element of the free file information
  list and returns it.
*/
struct file_info_int* free_file_infos_pop() {
  struct file_info_int* ret = free_file_infos.next;

  if (ret == &free_file_infos) {
    return NULL;
  }

  free_file_infos.next = ret->next;
  ret->next->prev = &free_file_infos;
  return ret;
}
