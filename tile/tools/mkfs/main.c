#define STRING_H
#define _GNU_SOURCE

#include <kernel/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define DIRECTORIES_SIZE 14

#define file_num_to_offset(num) (file_num_to_block_num(num) * FILE_BLOCK_SIZE + file_num_to_block_offset(num))

char* program;
char* optstring = ":b:";
char* directories[DIRECTORIES_SIZE] = {"bin", "boot", "dev", "etc", "lib", "media", "mnt", "opt", "run", "sbin", "srv", "tmp", "usr", "var"};

struct mkfs_context {
  void* addr;
  size_t next_file_info;
  size_t reserved_data_blocks;
  struct filesystem_info* info;
};

void usage() {
  char* usagestring = "[-b blocks-count] device";
  fprintf(stderr, "Usage: %s %s\n", program, usagestring);
  exit(EXIT_FAILURE);
}

/*
  data_blocks_begin returns the beginning data block number from the context
  "ctx".
*/
uint32_t data_blocks_begin(struct mkfs_context* ctx) {
  return ctx->info->size - ctx->info->file_infos_size;
}

/*
  free_data_blocks returns the number of data blocsk with a free data block
  list from the context "ctx".
*/
uint32_t free_data_blocks(struct mkfs_context* ctx) {
  return (data_blocks_begin(ctx) - 1) / FILESYSTEM_INFO_CACHE_SIZE;
}

/*
  write_file_info writes the file information "file" given the context "ctx".
*/
void write_file_info(struct mkfs_context* ctx, const struct file_info_ext* file) {
  *((struct file_info_ext*)((uint64_t)ctx->addr + file_num_to_offset(file->num))) = *file;
}

/*
  write_free_block_list writes a free block list to the location given by "ptr"
  from the context "ctx", block number "num" and the size of the list "size".
*/
void write_free_block_list(struct mkfs_context* ctx, void* ptr, size_t num, size_t size) {
  ((uint32_t*)(ptr))[0] = data_blocks_begin(ctx) + num + ctx->reserved_data_blocks;

  for (size_t i = 0; i < size - 1; ++i) {
    ((uint32_t*)(ptr))[i + 1] = data_blocks_begin(ctx) + free_data_blocks(ctx) + ctx->reserved_data_blocks + (num * (FILESYSTEM_INFO_CACHE_SIZE - 1)) + i;
  }
}

/*
  write_directory_info writes the directory information "directory" under
  "parent" from the context "ctx".
*/
void write_directory_info(struct mkfs_context* ctx, struct file_info_ext* parent, struct directory_info* directory) {
  size_t prev_block;
  size_t curr_block;
  size_t offset;

  prev_block = parent->size / sizeof(struct directory_info) / DIRECTORIES_PER_BLOCK;
  curr_block = (parent->size + sizeof(struct directory_info) - 1) / sizeof(struct directory_info) / DIRECTORIES_PER_BLOCK;

  /*
    If there are no blocks, or the current block is full, then allocate another
    one.
  */
  if (!parent->size || curr_block > prev_block) {
    parent->blocks[curr_block] = data_blocks_begin(ctx) + ctx->reserved_data_blocks;
    ++ctx->reserved_data_blocks;
  }

  offset = parent->blocks[curr_block] * FILE_BLOCK_SIZE + parent->size % (sizeof(struct directory_info) * DIRECTORIES_PER_BLOCK);
  *((struct directory_info*)((uint64_t)ctx->addr + offset)) = *directory;

  parent->size += sizeof(struct directory_info);
}

/*
  init_directory initializes the required directory entries of the directory
  "file" given the context "ctx".
*/
void init_directory(struct mkfs_context* ctx, struct file_info_ext* file) {
  struct directory_info directory;

  directory.num = file->num;
  strcpy(directory.name, ".");
  write_directory_info(ctx, file, &directory);
  strcpy(directory.name, "..");
  write_directory_info(ctx, file, &directory);
}

/*
  mkfs_mkdir makes a directory under "parent" with the name "name".
*/
struct file_info_ext mkfs_mkdir(struct mkfs_context* ctx, struct file_info_ext* parent, char* name) {
  struct file_info_ext file;
  struct directory_info directory;

  file.num = ctx->next_file_info;
  ++ctx->next_file_info;
  file.type = FT_DIRECTORY;
  file.size = 0;

  directory.num = file.num;
  strcpy(directory.name, ".");
  write_directory_info(ctx, &file, &directory);

  if (!parent) {
    directory.num = file.num;
  }
  else {
    directory.num = parent->num;
  }

  strcpy(directory.name, "..");
  write_directory_info(ctx, &file, &directory);

  if (parent) {
    directory.num = file.num;
    strcpy(directory.name, name);
    write_directory_info(ctx, parent, &directory);
  }

  write_file_info(ctx, &file);

  return file;
}

int main(int argc, char* argv[]) {
  int opt;
  char* device;
  size_t blocks_count = 4096;
  size_t device_size;
  int fd;
  void* addr;
  struct filesystem_info info;
  struct file_info_ext root;
  uint32_t free_blocks_begin;
  struct mkfs_context ctx;

  program = argv[0];

  while((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'b':
        blocks_count = strtoull(optarg, NULL, 10);
        break;
      default:
        usage();
    }
  }

  if (optind != argc - 1) {
    usage();
  }

  device = argv[optind];

  if (!blocks_count) {
    fprintf(stderr, "%s: error: invalid blocks count\n", program);
    exit(EXIT_FAILURE);
  }

  device_size = blocks_count * FILE_BLOCK_SIZE;

  fd = open(device, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  if (fd < 0) {
    fprintf(stderr, "%s: error: open failed\n", program);
    exit(EXIT_FAILURE);
  }

  fallocate(fd, 0, 0, device_size);
  addr = mmap(NULL, device_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if (addr == MAP_FAILED) {
    fprintf(stderr, "%s: error: mmap failed\n", program);
    exit(EXIT_FAILURE);
  }

  /* Initialize the context. */
  ctx.addr = addr;
  ctx.next_file_info = 1;
  ctx.reserved_data_blocks = 0;
  ctx.info = &info;

  /* Initialize the filesystem information. */
  info.size = blocks_count;
  info.free_blocks_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_block = 0;
  info.file_infos_size = (info.size - 1) / 2;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_file_info = 1;
  info.root_file_info = 1;

  /* Initialize the file information blocks. */
  for (size_t i = 0; i < info.file_infos_size; ++i) {
    for (size_t j = 0; j < FILE_INFO_PER_BLOCK; ++j) {
      struct file_info_ext file_info_ext;
      file_info_ext.num = i * FILE_INFO_PER_BLOCK + j + 1;
      file_info_ext.type = 0;
      file_info_ext.size = 0;
      ((struct file_info_ext*)((uint64_t)addr + i * FILE_BLOCK_SIZE))[j] = file_info_ext;
    }
  }

  /* Initialize the root filesystem. */
  root = mkfs_mkdir(&ctx, NULL, "/");

  for (size_t i = 0; i < DIRECTORIES_SIZE; ++i) {
    mkfs_mkdir(&ctx, &root, directories[i]);
  }

  write_file_info(&ctx, &root);

  /* Initialize the free file information list. */
  for (size_t i = 0; i < info.free_file_infos_size; ++i) {
    info.free_file_infos[i] = ctx.next_file_info + i;
  }

  /* Initialize the free data blocks. */
  write_free_block_list(&ctx, info.free_blocks, 0, FILESYSTEM_INFO_CACHE_SIZE);

  free_blocks_begin = (data_blocks_begin(&ctx) + ctx.reserved_data_blocks) * FILE_BLOCK_SIZE;

  for (size_t i = 0; i < free_data_blocks(&ctx) - 1; ++i) {
    write_free_block_list(&ctx, (void*)(free_blocks_begin + (uint64_t)addr + i * FILE_BLOCK_SIZE + 1), i + 1, FILESYSTEM_INFO_CACHE_SIZE);
  }

  /* Initialize the filesystem information block. */
  *((struct filesystem_info*)addr) = info;

  munmap(addr, device_size);
  close(fd);

  return 0;
}
