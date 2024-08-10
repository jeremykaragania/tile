#ifndef FILE_H
#define FILE_H

#include <kernel/mci.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

#define FILE_TABLE_SIZE 32
#define FILE_INFO_TABLE_SIZE 32
#define FILE_BLOCK_SIZE 512

enum file_status {
  FS_READ,
  FS_WRITE
};

enum file_type {
  FT_REGULAR,
  FT_DIRECTORY,
  FT_CHARACTER,
  FT_BLOCK,
  FT_FIFO
};

struct file_entry {
  int status;
  size_t offset;
};

struct file_info_entry {
  uint32_t blocks[15];
  int type;
  size_t size;
};

struct file_info_entry* lookup_file_info(const char* name);

#endif
