#ifndef FILE_H
#define FILE_H

#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <kernel/mci.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

#define FILE_TABLE_SIZE 32
#define FILE_INFO_TABLE_SIZE 32
#define FILE_INFO_PER_BLOCK FILE_BLOCK_SIZE / sizeof(struct file_info_entry)
#define FILESYSTEM_INFO_CACHE_SIZE 32

extern struct filesystem_info filesystem_info;

/*
  enum file_status represents the status of an operation on a file.
*/
enum file_status {
  FS_READ,
  FS_WRITE
};

/*
  enum file_type represents the type of a file.
*/
enum file_type {
  FT_REGULAR,
  FT_DIRECTORY,
  FT_CHARACTER,
  FT_BLOCK,
  FT_FIFO
};

/*
  struct filesystem_info represents the information of a filesystem. It tracks
  blocks and file information.
*/
struct filesystem_info {
  uint32_t size;
  uint32_t free_blocks_size;
  uint32_t free_blocks[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_block;
  uint32_t free_file_infos_size;
  uint32_t free_file_infos[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_file_info;
};

/*
  struct file_entry represents the information of a file which is currently
  being accessed.
*/
struct file_entry {
  int status;
  size_t offset;
};

/*
  struct file_info_entry represents the metadata of a file.
*/
struct file_info_entry {
  uint32_t blocks[15];
  int type;
  size_t size;
};

void filesystem_init();

struct file_info_entry* lookup_file_info(const char* name);

struct file_info_entry* file_info_get(uint32_t num);

#endif
