#define STRING_H

#include <kernel/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char command[] = "mkfs";
char usage[] = "Usage: mkfs device blocks-count";

/*
  write_free_block_list writes a free block list to the location given by "ptr"
  from the filesystem information "info", block number "num" and the size of
  the list "size".
*/
void write_free_block_list(struct filesystem_info* info, void* ptr, size_t num, size_t size) {
  uint32_t data_blocks_count = info->size - 1 - info->file_infos_size;
  uint32_t free_data_blocks_count = data_blocks_count / FILESYSTEM_INFO_CACHE_SIZE;

  ((uint32_t*)(ptr))[0] = info->file_infos_size + num + 1;

  for (size_t i = 0; i < size - 1; ++i) {
    ((uint32_t*)(ptr))[i + 1] = free_data_blocks_count + (num * (FILESYSTEM_INFO_CACHE_SIZE - 1)) + i;
  }
}

int main(int argc, char* argv[]) {
  char* device;
  size_t blocks_count;
  size_t data_blocks_count;
  size_t free_data_blocks_count;
  FILE* f;
  struct filesystem_info info;
  char block[FILE_BLOCK_SIZE];

  if (argc != 3) {
    puts(usage);
    return 0;
  }

  device = argv[1];
  blocks_count = strtoull(argv[2], NULL, 10);
  f = fopen(device, "wb");

  if (f == NULL) {
    return 0;
  }

  /* Initialize the filesystem information. */
  info.size = blocks_count;
  info.free_blocks_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_block = 0;
  info.file_infos_size = (info.size - 1) / 2;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_file_info = 0;
  info.root_file_info = 0;

  write_free_block_list(&info, info.free_blocks, 0, FILESYSTEM_INFO_CACHE_SIZE);

  for (size_t i = 0; i < info.free_file_infos_size; ++i) {
    info.free_file_infos[i] = i + 1;
  }

  /* Initialize the filesystem information block. */
  memset(block, 0, FILE_BLOCK_SIZE);
  fwrite(block, FILE_BLOCK_SIZE, 1, f);

  /* Initialize the file information blocks. */
  for (size_t i = 0; i < info.file_infos_size; ++i) {
    memset(block, 0, FILE_BLOCK_SIZE);

    for (size_t j = 0; j < FILE_INFO_PER_BLOCK; ++j) {
      struct file_info_ext file_info_ext;
      file_info_ext.num = i * FILE_INFO_PER_BLOCK + j + 1;
      file_info_ext.type = 0;
      file_info_ext.size = 0;
      ((struct file_info_ext*)block)[j] = file_info_ext;
    }

    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  data_blocks_count = info.size - 1 - info.file_infos_size;
  free_data_blocks_count = data_blocks_count / FILESYSTEM_INFO_CACHE_SIZE;

  /* Initialize the free data blocks. */
  for (size_t i = 0; i < free_data_blocks_count; ++i) {
    memset(block, 0, FILE_BLOCK_SIZE);
    ((uint32_t*)(block))[0] = FILESYSTEM_INFO_CACHE_SIZE;
    write_free_block_list(&info, (uint32_t*)block + 1, i + 1, FILESYSTEM_INFO_CACHE_SIZE);
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  /* Initialize the data blocks. */
  memset(block, 0, FILE_BLOCK_SIZE);

  for (size_t i = 0; i < info.size - 1 - info.file_infos_size - free_data_blocks_count; ++i) {
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  /* Initialize the filesystem information block. */
  rewind(f);
  fwrite(&info, sizeof(info), 1, f);

  return 0;
}
