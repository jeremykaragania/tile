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

      mci_read(entry.blocks[0] * FILE_BLOCK_SIZE, block);

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
