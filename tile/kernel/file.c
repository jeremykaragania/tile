#include <kernel/file.h>

struct filesystem_info filesystem_info;

/*
  filesystem_init initializes the filesystem. It assumes that the filesystem
  begins at the beginning address of the SD card.
*/
void filesystem_init() {
  char* block = memory_alloc(FILE_BLOCK_SIZE);

  /*
    The first block contains information about the filesystem. The memory
    layout is specified by struct filesystem_info.
  */
  mci_read(0, block);
  filesystem_info = *(struct filesystem_info*)block;
  memory_free(block);
}

/*
  file_info_int_get returns internal file information from an external file
  information number "file_info_num".
*/
struct file_info_int* file_info_get(uint32_t file_info_num) {
  struct file_info_int* ret = NULL;
  char* block = NULL;
  uint32_t block_num = 1 + (file_info_num - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_ext) * ((file_info_num - 1) % FILE_INFO_PER_BLOCK);

  block = memory_alloc(FILE_BLOCK_SIZE);
  ret = memory_alloc(sizeof(struct file_info_int));
  mci_read(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  ret->ext = *(struct file_info_ext*)(block + block_offset);
  memory_free(block);
  return ret;
}

/*
  file_info_int_put writes the external file information from the internal file
  information "file_info" to the SD card.
*/
void file_info_put(const struct file_info_int* file_info) {
  char* block = NULL;
  uint32_t block_num = 1 + (file_info->ext.num - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_ext) * ((file_info->ext.num - 1) % FILE_INFO_PER_BLOCK);

  block = memory_alloc(FILE_BLOCK_SIZE);
  mci_read(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  *(struct file_info_ext*)(block + block_offset) = file_info->ext;
  mci_write(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  memory_free(block);
}
