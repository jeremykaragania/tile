#include <kernel/file.h>

struct file_entry file_table[FILE_TABLE_SIZE];
struct file_info_entry file_info_table[FILE_INFO_TABLE_SIZE];
struct filesystem_info filesystem_info;

/*
  filesystem_init initializes the filesystem. It assumes that the filesystem
  begins at the beginning address of the SD card.
*/
void filesystem_init() {
  char* block = memory_alloc(FILE_BLOCK_SIZE);

  /*
    The first block contains information about the filesystem. The memory
    layout is specified by struct filesystem_info.
  */
  mci_read(0, block);
  filesystem_info = *(struct filesystem_info*)block;
  memory_free(block);
}

/*
  lookup_file_info searches the file information table "file_info_table" by a
  filename and returns the file's information.
*/
struct file_info_entry* lookup_file_info(const char* filename) {
  for (size_t i = 0; i < FILE_INFO_TABLE_SIZE; ++i) {
    struct file_info_entry entry = file_info_table[i];

    if (entry.type == FT_DIRECTORY) {
      char block[FILE_BLOCK_SIZE];

      mci_read(FILE_BLOCK_SIZE + entry.blocks[0] * FILE_BLOCK_SIZE, block);

      for (size_t j = (uint32_t)block; j < (uint32_t)block + entry.size; j += 16) {
        size_t number = *(uint16_t*)((char*)j);
        char* name = (char*)(j + 2);

        if (strcmp(name, filename) == 0) {
          return &file_info_table[number];
        }
      }
    }
  }

  return NULL;
}

/*
  file_info_get returns file information from a file information number
  "file_info_num".
*/
struct file_info_entry* file_info_get(uint32_t file_info_num) {
  char* block = NULL;
  struct file_info_entry* ret = NULL;
  uint32_t block_num = 1 + (file_info_num - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_entry) * ((file_info_num - 1) % FILE_INFO_PER_BLOCK);

  for (size_t i = 0; i < FILE_INFO_TABLE_SIZE; ++i) {
    if (!file_info_table[i].type) {
      ret = &file_info_table[i];
      break;
    }
  }

  if (ret == NULL) {
    return NULL;
  }

  block = memory_alloc(FILE_BLOCK_SIZE);
  mci_read(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  *ret = *(struct file_info_entry*)(block + block_offset);
  memory_free(block);
  return ret;
}

/*
  file_info put writes the file information "file_info" to the SD card.
*/
void file_info_put(const struct file_info_entry* file_info) {
  char* block = NULL;
  uint32_t block_num = 1 + (file_info->number - 1) / FILE_INFO_PER_BLOCK;
  uint32_t block_offset = sizeof(struct file_info_entry) * ((file_info->number - 1) % FILE_INFO_PER_BLOCK);

  block = memory_alloc(FILE_BLOCK_SIZE);
  mci_read(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  *(struct file_info_entry*)(block + block_offset) = *file_info;
  mci_write(FILE_BLOCK_SIZE + FILE_BLOCK_SIZE * block_num, block);
  memory_free(block);
}
