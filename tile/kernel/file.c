#include <kernel/file.h>

struct file_info file_table[32];
struct inode inode_table[32];

struct inode* lookup_inode(const char* name) {
  for (size_t i = 0; i < 32; ++i) {
    if (strcmp(inode_table[i].name, name) == 0) {
      return &inode_table[i];
    }
  }

  return NULL;
}
