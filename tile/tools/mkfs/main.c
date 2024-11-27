#define STRING_H

#include <kernel/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char command[] = "mkfs";
char usage[] = "Usage: mkfs device blocks-count";

int main(int argc, char* argv[]) {
  char* device;
  size_t blocks_count;
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

  for (size_t i = 0; i < info.free_blocks_size; ++i) {
    info.free_blocks_cache[i] = i + 2;
  }

  info.next_free_block = 0;
  info.file_infos_size = (blocks_count - 1) / 2;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;

  for (size_t i = 0; i < info.free_file_infos_size; ++i) {
    info.free_file_infos_cache[i] = i + 1;
  }

  info.next_free_file_info = 0;

  /* Initialize the filesystem information block. */
  memset(block, 0, FILE_BLOCK_SIZE);
  *(struct filesystem_info*)block = info;
  fwrite(block, FILE_BLOCK_SIZE, 1, f);

  /* Initialize the file information blocks. */
  for (size_t i = 0; i < info.file_infos_size; ++i) {
    memset(block, 0, FILE_BLOCK_SIZE);

    for (size_t j = 0; j < FILE_INFO_PER_BLOCK; ++j) {
      struct file_info_ext file_info_ext;
      file_info_ext.num = i * FILE_INFO_PER_BLOCK + j + 1;
      ((struct file_info_ext*)block)[j] = file_info_ext;
    }

    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  /* Initialize the data blocks. */
  memset(block, 0, FILE_BLOCK_SIZE);

  for (size_t i = 0; i < (blocks_count - 1) - info.file_infos_size; ++i) {
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  return 0;
}
