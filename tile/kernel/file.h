#ifndef FILE_H
#define FILE_H

#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <kernel/mci.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>

#define FILE_INFO_PER_BLOCK (FILE_BLOCK_SIZE / sizeof(struct file_info_ext))
#define FILE_INFO_CACHE_SIZE 32
#define FILESYSTEM_INFO_CACHE_SIZE 32

extern struct filesystem_info filesystem_info;

/*
  "file_info_cache" is a cache for internal file information. It is the memory
  backing all internal file information read from secondary memory.
*/
extern struct file_info_int* file_info_cache;

/*
  "free_file_infos" is a doubly linked list which stores the internal file
  information in "file_info_cache" which are not being used.
*/
extern struct file_info_int free_file_infos;

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
  FT_REGULAR = 1,
  FT_DIRECTORY,
  FT_CHARACTER,
  FT_BLOCK,
  FT_FIFO
};

/*
  enum_file_access represents the access permissions of a file.
*/
enum file_access {
  F_READ,
  F_WRITE,
  F_EXECUTE
};

/*
  struct filesystem_info represents the information of a filesystem. It is
  always the first block of the filesystem and tracks blocks and external file
  information.
*/
struct filesystem_info {
  uint32_t size;
  uint32_t free_blocks_size;
  uint32_t free_blocks_cache[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_block;
  uint32_t file_infos_size;
  uint32_t free_file_infos_size;
  uint32_t free_file_infos_cache[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_file_info;
};

/*
  struct file_info_ext represents external file information for file
  information that is in secondary memory. They are like UNIX disk inodes.
*/
struct file_info_ext {
  uint32_t num;
  uint32_t blocks[15];
  int type;
  size_t size;
};

/*
  struct file_info_int represents internal file information for file
  information that is in primary memory. They are like UNIX in-core inodes.
*/
struct file_info_int {
  struct file_info_ext ext;
  int status;
  size_t offset;
  struct file_info_int* next;
  struct file_info_int* prev;
};

void filesystem_init();

struct file_info_int* file_info_get(uint32_t file_info_num);
void file_info_put(const struct file_info_int* file_info);

void free_file_infos_push(struct file_info_int* file_info);
struct file_info_int* free_file_infos_pop();

#endif
