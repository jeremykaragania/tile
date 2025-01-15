#ifndef FILE_H
#define FILE_H

#include <kernel/buffer.h>
#include <kernel/asm/file.h>
#include <kernel/memory.h>
#include <kernel/process.h>
#include <lib/string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define FILE_INFO_PER_BLOCK (FILE_BLOCK_SIZE / sizeof(struct file_info_ext))
#define FILE_INFO_CACHE_SIZE 32
#define FILE_TABLE_SIZE 32
#define FILESYSTEM_INFO_CACHE_SIZE 32
#define FILE_NAME_SIZE 28

#define BLOCK_NUMS_PER_BLOCK (FILE_BLOCK_SIZE / sizeof(uint32_t))
#define DIRECTORIES_PER_BLOCK (FILE_BLOCK_SIZE / sizeof(struct directory_info))

/*
  A file's data is accessed through four levels of indirection. Zeroth level
  blocks contain direct data, first level blocks contain the numbers of zeroth
  level blocks, second level blocks contain the numbers of first level blocks,
  and third level blocks contain the numbers of second level blocks.
*/
#define L0_BLOCKS_SIZE 10
#define L1_BLOCKS_SIZE 1
#define L2_BLOCKS_SIZE 1
#define L3_BLOCKS_SIZE 1

#define L0_BLOCKS_COUNT 1
#define L1_BLOCKS_COUNT BLOCK_NUMS_PER_BLOCK
#define L2_BLOCKS_COUNT (BLOCK_NUMS_PER_BLOCK * BLOCK_NUMS_PER_BLOCK)
#define L3_BLOCKS_COUNT (BLOCK_NUMS_PER_BLOCK * BLOCK_NUMS_PER_BLOCK * BLOCK_NUMS_PER_BLOCK)

#define L0_BLOCKS_END (L0_BLOCKS_SIZE * L0_BLOCKS_COUNT * FILE_BLOCK_SIZE - 1)
#define L1_BLOCKS_END (L0_BLOCKS_END + L1_BLOCKS_SIZE * L1_BLOCKS_COUNT * FILE_BLOCK_SIZE)
#define L2_BLOCKS_END (L1_BLOCKS_END + L2_BLOCKS_SIZE * L2_BLOCKS_COUNT * FILE_BLOCK_SIZE)
#define L3_BLOCKS_END (L2_BLOCKS_END + L3_BLOCKS_SIZE * L3_BLOCKS_COUNT * FILE_BLOCK_SIZE)

#define FILE_INFO_BLOCKS_SIZE (L0_BLOCKS_SIZE + L1_BLOCKS_SIZE + L2_BLOCKS_SIZE + L3_BLOCKS_SIZE)

#define file_num_to_block_num(num) (1 + (num - 1) / FILE_INFO_PER_BLOCK)
#define file_num_to_block_offset(num) (sizeof(struct file_info_ext) * ((num - 1) % FILE_INFO_PER_BLOCK))
#define blocks_in_file(size) ((size + FILE_BLOCK_SIZE - 1) / FILE_BLOCK_SIZE)

extern const char* current_directory;
extern const char* parent_directory;

extern struct filesystem_info filesystem_info;

/*
  "file_info_pool" is a pool for internal file information. It is the memory
  backing all internal file information read from secondary memory.
*/
extern struct file_info_int* file_info_pool;

/*
  "file_infos" is a cache for internal file information.
*/
extern struct file_info_int file_infos;

/*
  "free_file_infos" is a doubly linked list which stores the internal file
  information in "file_info_pool" which are not being used.
*/
extern struct file_info_int free_file_infos;

/*
  enum file_status represents the status of an operation on a file.
*/
enum file_status {
  FS_READ = (1 << 0),
  FS_WRITE = (1 << 1)
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
  file_open_flag represents the flags which can be passed when opening a file.
  They are the same as the POSIX ones.
*/
enum file_open_flag {
  O_RDONLY = (1 << 0),
  O_RDWR = 3,
  O_WRONLY = (1 << 1),
  O_APPEND = (1 << 2),
  O_CREAT = (1 << 3),
  O_EXCL = (1 << 4),
  O_TRUNC = (1 << 5)
};

/*
  struct filesystem_info represents the information of a filesystem. It is
  always the first block of the filesystem and tracks blocks and external file
  information.
*/
struct filesystem_info {
  uint32_t size;
  uint32_t free_blocks_size;
  uint32_t free_blocks[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_block;
  uint32_t file_infos_size;
  uint32_t free_file_infos_size;
  uint32_t free_file_infos[FILESYSTEM_INFO_CACHE_SIZE];
  uint8_t next_free_file_info;
  uint32_t root_file_info;
};

/*
  struct filesystem_addr the address of a location within the filesystem. It is
  specified by a block number "num", and an offset "offset" within that block.
*/
struct filesystem_addr {
  uint32_t num;
  uint32_t offset;
};

/*
  struct block_info represents block information. It contains a block's level
  and index.
*/
struct block_info {
  uint32_t level;
  uint32_t index;
};

/*
  struct file_info_ext represents external file information for file
  information that is in secondary memory. They are like UNIX disk inodes.
*/
struct file_info_ext {
  uint32_t num;
  uint32_t blocks[FILE_INFO_BLOCKS_SIZE];
  int32_t type;
  uint32_t size;
};

/*
  struct file_info_int represents internal file information for file
  information that is in primary memory. They are like UNIX in-core inodes.
*/
struct file_info_int {
  struct file_info_ext ext;
  int status;
  struct file_info_int* next;
  struct file_info_int* prev;
};

/*
  directory_info represents the information which is stored in a directory. It
  contains a file information number "num" and the name of the associated file
  "name".
*/
struct directory_info {
  uint32_t num;
  char name[FILE_NAME_SIZE];
};

/*
  struct file_table_entry represents an entry in the file table.
*/
struct file_table_entry {
  int status;
  uint32_t offset;
  struct file_info_int* file_int;
};

void filesystem_init();

int file_open(const char* name, int flags);
int file_read(int fd, void* buf, size_t count);
int file_write(int fd, const void* buf, size_t count);
int file_close(int fd);
int file_mknod(const char* pathname, int type);
int file_creat(const char* pathname, int flags);
int file_seek(int fd, size_t offset);

int get_file_descriptor(const struct file_table_entry* file_tab);

void file_resize(struct file_info_int* file, size_t size);
void file_push_blocks(struct file_info_int* file, size_t count);
void file_pop_blocks(struct file_info_int* file, size_t count);

struct filesystem_addr file_offset_to_addr(const struct file_info_int* info, uint32_t offset);
struct block_info file_offset_to_block(uint32_t offset);
uint32_t block_num_index(size_t level, uint32_t offset);

struct file_info_int* name_to_file(const char* name);
void get_pathname_info(const char* pathname, char* parent, char* file);
char* normalize_pathname(char* pathname);

struct file_info_int* file_get(uint32_t file_info_num);
void file_put(struct file_info_int* file_info);

void file_push(struct file_info_int* list, struct file_info_int* file_info);
struct file_info_int* file_pop(struct file_info_int* list);
void file_remove(struct file_info_int* list, struct file_info_int* file_info);

struct file_info_int* file_alloc();
void file_free(const struct file_info_int* file_info);

struct buffer_info* block_alloc();
void block_free(struct buffer_info* buffer_info);

struct filesystem_addr file_to_addr(uint32_t file_info_num);

struct file_table_entry* file_table_alloc();

#endif
