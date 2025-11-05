/*
  mkfs is a tool for building Tile filesystems. It initializes the root
  directory, file information, and blocks.
*/

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

#define FS_ACCESS (FA_READ_OWNER | FA_WRITE_OWNER | FA_EXEC_OWNER | \
                   FA_READ_GROUP | FA_EXEC_GROUP | \
                   FA_READ_OTHERS | FA_EXEC_OTHERS)

#define DIRECTORIES_SIZE 14

#define file_num_to_offset(num) (file_num_to_block_num(num) * BLOCK_SIZE + file_num_to_block_offset(num))
#define get_file(ctx, num) ((struct file_info_ext*)((uint64_t)(ctx)->device_addr + file_num_to_offset(num)))
#define get_block(ctx, num) ((void*)(num * BLOCK_SIZE + (uint64_t)ctx->device_addr))

char* program;
char* optstring = ":b:i:";
char* directories[DIRECTORIES_SIZE] = {"bin", "boot", "dev", "etc", "lib", "media", "mnt", "opt", "run", "sbin", "srv", "tmp", "usr", "var"};
struct file_info_ext* directory_infos[DIRECTORIES_SIZE];
struct file_owner root_owner = {0, 0};

struct mkfs_context {
  void* device_addr;
  size_t next_file_info;
  size_t reserved_data_blocks;
  struct filesystem_info* info;
};

/*
  get_directory return the file information associated with a directory in the
  root directory from its name "name".
*/
struct file_info_ext* get_directory(char* name) {
  for (size_t i = 0; i < DIRECTORIES_SIZE; ++i) {
    if (strcmp(name, directories[i]) == 0) {
      return directory_infos[i];
    }
  }

  return NULL;
}

/*
  file_offset_to_block converts an offset in a file "offset" to a block's level
  and index.
*/
struct block_info file_offset_to_block(uint32_t offset) {
  struct block_info ret = {0, 0};

  /*
    We determine which block level "offset" is at and the corresponding block
    index inside "file_info".
  */
  if (offset <= L0_BLOCKS_END) {
    ret.level = 0;
    ret.index = offset / BLOCK_SIZE;
  }
  else if (offset <= L1_BLOCKS_END) {
    ret.level = 1;
    ret.index = L0_BLOCKS_SIZE;
  }
  else if (offset <= L2_BLOCKS_END) {
    ret.level = 2;
    ret.index = L0_BLOCKS_SIZE + L1_BLOCKS_SIZE;
  }
  else if (offset <= L3_BLOCKS_END) {
    ret.level = 3;
    ret.index = L0_BLOCKS_SIZE + L1_BLOCKS_SIZE + L2_BLOCKS_SIZE;
  }

  return ret;
}

/*
  block_num_index returns the index of a block number at the block level
  "level" and a file byte offset "offset".
*/
uint32_t block_num_index(size_t level, uint32_t offset) {
  size_t begin;
  size_t step;

  /*
    At each block level, the beginning file byte offset along with how many
    bytes a block represents.
  */
  switch (level) {
    case 1:
      begin = L0_BLOCKS_END + 1;
      step = BLOCK_SIZE;
      break;
    case 2:
      begin = L1_BLOCKS_END + 1;
      step = L2_BLOCKS_COUNT * BLOCK_SIZE;
      break;
    case 3:
      begin = L2_BLOCKS_END + 1;
      step = L3_BLOCKS_COUNT * BLOCK_SIZE;
      break;
    default:
      return 0;
  }

  return (offset - begin) / step % BLOCK_NUMS_PER_BLOCK;
}

/*
  usage displays usage information if mkfs was used incorrectly.
*/
void usage() {
  char* usagestring = "[-b blocks-count] [-i init] device";
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
  alloc_block allocates a data block from the context "ctx", and returns its
  number.
*/
size_t alloc_block(struct mkfs_context* ctx) {
  size_t ret = ctx->reserved_data_blocks;
  ++ctx->reserved_data_blocks;
  return ret;
}

/*
  alloc_file allocates external file information from the context "ctx", and
  returns a pointer to it.
*/
struct file_info_ext* alloc_file(struct mkfs_context* ctx) {
  size_t num = ctx->next_file_info;
  ++ctx->next_file_info;
  return get_file(ctx, num);
}

/*
  push_block pushes a block to the file "file" and returns its block number.
*/
uint32_t push_block(struct mkfs_context* ctx, struct file_info_ext* file) {
  size_t offset = BLOCK_SIZE * blocks_in_file(file->size);
  struct block_info block_info = file_offset_to_block(offset);
  uint32_t block_num = file->num;
  void* block = get_block(ctx, block_num);
  uint32_t ret;

  /*
    If the previous block index is different from the current block index, or
    this is the first block then we allocate it.
  */
  if (file_offset_to_block(offset - BLOCK_SIZE).index != block_info.index || !block_info.index) {
    ret = alloc_block(ctx);
    file->blocks[block_info.index] = ret;
  }

  /*
    Starting from the first block level, we iterate up to the level of the
    current block, allocating intermediate blocks where neccessary.
  */
  for (size_t i = 0; i < block_info.level; ++i) {
    size_t prev_index = block_num_index(block_info.level - i, offset - BLOCK_SIZE);
    size_t curr_index = block_num_index(block_info.level - i, offset);

    block = get_block(ctx, block_num);

    /*
      If the previous block number index is different from the current block
      index, then we allocate one. This is a hack which works due to how a
      block number index depends on the offset and the level.
    */
    if (prev_index != curr_index) {
      ret = alloc_block(ctx);
      ((uint32_t*)block)[curr_index] = ret;
    }

    block_num = ((uint32_t*)block)[curr_index];
  }

  file->size += BLOCK_SIZE;

  return ret;
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

  offset = parent->blocks[curr_block] * BLOCK_SIZE + parent->size % (sizeof(struct directory_info) * DIRECTORIES_PER_BLOCK);
  *((struct directory_info*)((uint64_t)ctx->device_addr + offset)) = *directory;

  parent->size += sizeof(struct directory_info);
}

/*
  write_file_to_directory writes the file "src" to the directory "dest" with
  the name "name".
*/
void write_file_to_directory(struct mkfs_context* ctx, struct file_info_ext* dest, struct file_info_ext* src, char* name) {
  struct directory_info directory;

  directory.num = src->num;
  strncpy(directory.name, name, FILE_NAME_SIZE);

  write_directory_info(ctx, dest, &directory);
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
struct file_info_ext* mkfs_mkdir(struct mkfs_context* ctx, struct file_info_ext* parent, char* name) {
  struct file_info_ext* file;
  struct directory_info directory;

  file = alloc_file(ctx);
  file->type = FT_DIRECTORY;
  file->size = 0;
  file->owner = root_owner;
  file->access = FS_ACCESS;

  directory.num = file->num;
  strcpy(directory.name, ".");
  write_directory_info(ctx, file, &directory);

  if (!parent) {
    directory.num = file->num;
  }
  else {
    directory.num = parent->num;
  }

  strcpy(directory.name, "..");
  write_directory_info(ctx, file, &directory);

  if (parent) {
    directory.num = file->num;
    strcpy(directory.name, name);
    write_directory_info(ctx, parent, &directory);
  }

  return file;
}

/*
  copy_file copies "size" bytes from the buffer "addr" into the file "file".
*/
void copy_file(struct mkfs_context* ctx, struct file_info_ext* file, void* addr, size_t size) {
  size_t blocks = blocks_in_file(size);
  size_t curr_size = size;

  for (size_t i = 0; i < blocks; ++i) {
    uint32_t block = push_block(ctx, file);
    uint32_t count = curr_size >= BLOCK_SIZE ? BLOCK_SIZE : curr_size;

    memcpy(get_block(ctx, block), (void*)((uint64_t)addr + i * BLOCK_SIZE), count);
    curr_size -= count;
  }

  /*
    While pushing blocks we've already been incrementing the file size, but it
    will be block aligned.
  */
  file->size = size;
}

int main(int argc, char* argv[]) {
  int opt;
  char* device_path = NULL;
  int device_fd;
  void* device_addr;
  size_t device_size;
  char* init_path = NULL;
  int init_fd;
  void* init_addr;
  size_t init_size;
  struct file_info_ext* init;
  struct stat sb;
  size_t blocks_count = 4096;
  struct filesystem_info info;
  struct file_info_ext* root;
  uint32_t free_blocks_begin;
  struct mkfs_context ctx;

  program = argv[0];

  while((opt = getopt(argc, argv, optstring)) > 0) {
    switch (opt) {
      case 'b':
        blocks_count = strtoull(optarg, NULL, 10);
        break;
      case 'i':
        init_path = optarg;
        break;
      default:
        usage();
    }
  }

  if (optind != argc - 1) {
    usage();
  }

  if (!blocks_count) {
    fprintf(stderr, "%s: error: invalid blocks count\n", program);
    exit(EXIT_FAILURE);
  }

  /*
    Initialize the output file and an optional input init file.
  */
  device_path = argv[optind];
  device_fd = open(device_path, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);

  if (device_fd < 0) {
    fprintf(stderr, "%s: error: open failed\n", program);
    exit(EXIT_FAILURE);
  }

  device_size = blocks_count * BLOCK_SIZE;
  fallocate(device_fd, 0, 0, device_size);
  device_addr = mmap(NULL, device_size, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd, 0);

  if (device_addr == MAP_FAILED) {
    fprintf(stderr, "%s: error: mmap failed\n", program);
    exit(EXIT_FAILURE);
  }

  close(device_fd);

  if (init_path) {
    init_fd = open(init_path, O_RDONLY);

    if (init_fd < 0) {
      fprintf(stderr, "%s: error: open failed\n", program);
      exit(EXIT_FAILURE);
    }

    if (fstat(init_fd, &sb) < 0) {
      fprintf(stderr, "%s: error: fstat failed\n", program);
    }

    init_size = sb.st_size;
    init_addr = mmap(NULL, init_size, PROT_READ, MAP_SHARED, init_fd, 0);

    if (init_addr == MAP_FAILED) {
      fprintf(stderr, "%s: error: mmap failed\n", program);
      exit(EXIT_FAILURE);
    }

    close(init_fd);
  }

  /* Initialize the context. */
  ctx.device_addr = device_addr;
  ctx.next_file_info = 1;
  ctx.reserved_data_blocks = 0;
  ctx.info = &info;

  /* Initialize the filesystem information. */
  info.size = blocks_count;
  info.free_blocks_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_block = info.free_blocks_size - 1;
  info.file_infos_size = (info.size - 1) / 2;
  info.free_file_infos_size = FILESYSTEM_INFO_CACHE_SIZE;
  info.next_free_file_info = 1;
  info.root_file_info = 1;

  /* Initialize the file information blocks. */
  for (size_t i = 0; i < info.file_infos_size; ++i) {
    for (size_t j = 0; j < FILE_INFO_PER_BLOCK; ++j) {
      size_t file_num = i * FILE_INFO_PER_BLOCK + j + 1;
      struct file_info_ext* file= get_file(&ctx, file_num);
      file->num = file_num;
    }
  }

  /* Initialize the root filesystem and the init file. */
  root = mkfs_mkdir(&ctx, NULL, "/");

  for (size_t i = 0; i < DIRECTORIES_SIZE; ++i) {
    directory_infos[i] = mkfs_mkdir(&ctx, root, directories[i]);
  }

  if (init_path) {
    struct file_info_ext* sbin = get_directory("sbin");

    init = alloc_file(&ctx);
    init->type = FT_REGULAR;
    init->size = 0;
    init->owner = root_owner;
    init->access = FS_ACCESS;

    write_file_to_directory(&ctx, sbin, init, "init");
    copy_file(&ctx, init, init_addr, init_size);
  }

  /* Initialize the free file information list. */
  for (size_t i = 0; i < info.free_file_infos_size; ++i) {
    info.free_file_infos[i] = ctx.next_file_info + i;
  }

  /* Initialize the free data blocks. */
  write_free_block_list(&ctx, info.free_blocks, 0, FILESYSTEM_INFO_CACHE_SIZE);

  free_blocks_begin = (data_blocks_begin(&ctx) + ctx.reserved_data_blocks) * BLOCK_SIZE;

  /*
    Each free data block contains a list of other free data blocks. The length
    of the list is stored first in a block. The first free data block in the
    list also contains a list of data blocks which is how free data blocks are
    linked.
  */
  if (free_data_blocks(&ctx)) {
    for (size_t i = 0; i < free_data_blocks(&ctx) - 1; ++i) {
      uint32_t* block = (uint32_t*)((uint64_t)device_addr + free_blocks_begin + i * BLOCK_SIZE);
      size_t size = FILESYSTEM_INFO_CACHE_SIZE;

      *block = size;
      write_free_block_list(&ctx, block + 1, i + 1, size);
    }
  }

  /* Initialize the filesystem information block. */
  *((struct filesystem_info*)device_addr) = info;

  munmap(device_addr, device_size);

  return 0;
}
