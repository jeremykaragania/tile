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

  info.size = blocks_count;
  info.free_blocks_size = FILESYSTEM_INFO_CACHE_SIZE;
  memset(info.free_blocks_cache, 0, sizeof(info.free_blocks_cache[0]) * FILESYSTEM_INFO_CACHE_SIZE);
  info.next_free_block = 0;
  info.file_infos_size = (blocks_count - 1) / 2 * FILE_INFO_PER_BLOCK;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;
  memset(info.free_file_infos_cache, 0, sizeof(info.free_file_infos_cache[0]) * FILESYSTEM_INFO_CACHE_SIZE);
  info.next_free_file_info = 0;

  memset(block, 0, FILE_BLOCK_SIZE);
  *(struct filesystem_info*)block = info;
  fwrite(block, FILE_BLOCK_SIZE, 1, f);
  memset(block, 0, FILE_BLOCK_SIZE);

  for (size_t i = 0; i < blocks_count - 1; ++i) {
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  return 0;
}
