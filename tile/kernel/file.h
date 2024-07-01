#ifndef FILE_H
#define FILE_H

#include <stddef.h>

enum connection_status {
  FILE_READ,
  FILE_WRITE
};

struct file_info {
  int status;
  size_t offset;
  struct inode* inode;
};

struct inode {
  char* name;
};

struct directory_entry {
  char* filename;
  struct inode* inode;
};

#endif
