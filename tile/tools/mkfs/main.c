#define STRING_H

#include <kernel/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DIRECTORIES_SIZE 14

#define file_num_to_offset(num) (file_num_to_block_num(num) * FILE_BLOCK_SIZE + file_num_to_block_offset(num))

char command[] = "mkfs";
char usage[] = "Usage: mkfs device blocks-count";
char* directories[DIRECTORIES_SIZE] = {"bin", "boot", "dev", "etc", "lib", "media", "mnt", "opt", "run", "sbin", "srv", "tmp", "usr", "var"};

struct mkfs_context {
  FILE* file;
  size_t next_file_info;
  size_t reserved_data_blocks;
  struct filesystem_info* info;
};

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
  fseek(ctx->file, file_num_to_offset(file->num), SEEK_SET);
  fwrite(file, sizeof(struct file_info_ext), 1, ctx->file);
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
  curr_block = (parent->size + sizeof(struct directory_info)) / sizeof(struct directory_info) / DIRECTORIES_PER_BLOCK;

  /*
    If there are no blocks, or the current block is full, then allocate another
    one.
  */
  if (!parent->size || curr_block > prev_block) {
    parent->blocks[curr_block] = data_blocks_begin(ctx) + ctx->reserved_data_blocks;
    ++ctx->reserved_data_blocks;
  }

  parent->size += sizeof(struct directory_info);

  offset = parent->blocks[curr_block] * FILE_BLOCK_SIZE + parent->size % FILE_BLOCK_SIZE;
  fseek(ctx->file, offset, SEEK_SET);
  fwrite(&directory, sizeof(struct directory_info), 1, ctx->file);
}

/*
  mkdir makes a directory under "parent" with the name "name".
*/
void mkdir(struct mkfs_context* ctx, struct file_info_ext* parent, char* name) {
  struct file_info_ext file;
  struct directory_info directory;

  file.num = ctx->next_file_info;
  ++ctx->next_file_info;
  file.type = FT_DIRECTORY;

  directory.num = file.num;
  strcpy(directory.name, name);

  write_file_info(ctx, &file);
  write_directory_info(ctx, parent, &directory);
}

int main(int argc, char* argv[]) {
  char* device;
  size_t blocks_count;
  FILE* f;
  struct filesystem_info info;
  char block[FILE_BLOCK_SIZE];
  struct file_info_ext root;
  struct mkfs_context ctx;

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

  /* Initialize the context. */
  ctx.file = f;
  ctx.next_file_info = 2;
  ctx.reserved_data_blocks = 0;
  ctx.info = &info;

  /* Initialize the filesystem information. */
  info.size = blocks_count;
  info.free_blocks_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_block = 0;
  info.file_infos_size = (info.size - 1) / 2;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_file_info = 0;
  info.root_file_info = 1;

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

  /* Initialize the root filesystem. */
  root.num = 1;
  root.type = FT_DIRECTORY;
  root.size = 0;

  for (size_t i = 0; i < DIRECTORIES_SIZE; ++i) {
    mkdir(&ctx, &root, directories[i]);
  }

  write_file_info(&ctx, &root);

  /* Initialize the free data blocks. */
  write_free_block_list(&ctx, info.free_blocks, 0, FILESYSTEM_INFO_CACHE_SIZE);

  fseek(f, (data_blocks_begin(&ctx) + ctx.reserved_data_blocks) * FILE_BLOCK_SIZE, SEEK_SET);

  for (size_t i = 0; i < free_data_blocks(&ctx) - 1; ++i) {
    memset(block, 0, FILE_BLOCK_SIZE);
    ((uint32_t*)(block))[0] = FILESYSTEM_INFO_CACHE_SIZE;
    write_free_block_list(&ctx, (uint32_t*)block + 1, i + 1, FILESYSTEM_INFO_CACHE_SIZE);
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  /* Initialize the data blocks. */
  memset(block, 0, FILE_BLOCK_SIZE);

  for (size_t i = 0; i < info.size - data_blocks_begin(&ctx) - ctx.reserved_data_blocks - free_data_blocks(&ctx) + 1; ++i) {
    fwrite(block, FILE_BLOCK_SIZE, 1, f);
  }

  /* Initialize the filesystem information block. */
  rewind(f);
  fwrite(&info, sizeof(info), 1, f);

  return 0;
}
