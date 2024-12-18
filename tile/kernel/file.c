#include <kernel/file.h>

struct filesystem_info filesystem_info;
struct file_info_int* file_info_pool;
struct file_info_int file_infos;
struct file_info_int free_file_infos;

/*
  filesystem_init initializes the filesystem and the relevant structures used
  by the kernel. It assumes that the filesystem begins at the beginning address
  of the SD card.
*/
void filesystem_init() {
  struct buffer_info* buffer_info = NULL;
  struct file_info_int* curr;

  /*
    The first block contains information about the filesystem. The memory
    layout is specified by struct filesystem_info.
  */
  buffer_info = buffer_get(0);
  filesystem_info = *(struct filesystem_info*)buffer_info->data;

  /*
    Initialize the file information pool and the free file information list.
  */
  file_info_pool = memory_alloc(sizeof(struct file_info_int) * FILE_INFO_CACHE_SIZE);

  file_infos.next = &file_infos;
  file_infos.prev = &file_infos;

  free_file_infos.next = &file_info_pool[0];
  free_file_infos.prev = &file_info_pool[FILE_INFO_CACHE_SIZE - 1];

  curr = free_file_infos.next;

  for (size_t i = 0; i < FILE_INFO_CACHE_SIZE; ++i) {
    if (i == FILE_INFO_CACHE_SIZE - 1) {
      curr->next = &free_file_infos;
    }
    else {
      curr->next = &file_info_pool[i + 1];
    }

    curr->prev = &file_info_pool[i - 1];
    curr = curr->next;
  }
}

/*
  file_offset_to_addr returns the filesystem address at the byte offset
  "offset" in the file represented by the file information "info".
*/
struct filesystem_addr file_offset_to_addr(const struct file_info_int* file_info, uint32_t offset) {
  struct filesystem_addr ret = {0, 0};
  struct buffer_info* buffer_info;
  size_t level;
  uint32_t blocks_index;

  /*
    We determine which block level "offset" is at and the corresponding block
    index inside "file_info".
  */
  if (offset < L0_BLOCKS_END) {
    level = 0;
    blocks_index = offset / FILE_BLOCK_SIZE;
  }
  else if (offset < L1_BLOCKS_END) {
    level = 1;
    blocks_index = L0_BLOCKS_SIZE;
  }
  else if (offset < L2_BLOCKS_END) {
    level = 2;
    blocks_index = L0_BLOCKS_SIZE + L1_BLOCKS_SIZE;
  }
  else if (offset < L3_BLOCKS_END) {
    level = 3;
    blocks_index = L0_BLOCKS_SIZE + L1_BLOCKS_SIZE + L2_BLOCKS_SIZE;
  }
  else {
    return ret;
  }

  ret.num = file_info->ext.blocks[blocks_index];
  ret.offset = offset % 512;

  if (level) {
    for (size_t i = 0; i < level; ++i) {
      buffer_info = buffer_get(ret.num);
      ret.num = ((uint32_t*)buffer_info->data)[next_block_index(level - i, offset)];
      buffer_put(buffer_info);
    }
  }

  return ret;
}

/*
  next_block_index returns the index of a block number at the next block level
  from a block level "level" and a file byte offset "offset".
*/
uint32_t next_block_index(size_t level, uint32_t offset) {
  uint32_t begin;
  uint32_t step;

  /*
    At each block level, the beginning file byte offset along with how many
    bytes a block represents.
  */
  switch (level) {
    case 1:
      begin = L0_BLOCKS_END;
      step = L1_BLOCKS_COUNT * FILE_BLOCK_SIZE;
      break;
    case 2:
      begin = L1_BLOCKS_END;
      step = L2_BLOCKS_COUNT * FILE_BLOCK_SIZE;
      break;
    case 3:
      begin = L2_BLOCKS_END;
      step = L3_BLOCKS_COUNT * FILE_BLOCK_SIZE;
      break;
    default:
      return 0;
  }

  /*
    We imagine that we are at the beginning address and determine which block
    index contains the "offset".
  */
  for (size_t i = 0; i < BLOCK_NUMS_PER_BLOCK; ++i) {
    if (begin + i * step >= offset) {
      return i;
    }
  }

  return 0;
}

/*
  name_to_file returns internal file information from a path name "name".
*/
struct file_info_int* name_to_file(const char* name) {
  uint32_t num = filesystem_info.root_file_info;
  char component[FILE_NAME_SIZE];
  struct file_info_int* f;
  size_t i = FILE_NAME_SIZE;

  if (*name == '/') {
    num = filesystem_info.root_file_info;
  }
  else {
    num = current_process()->file_num;
  }

  f = file_get(num);

  while (*name) {
    int found = 0;
    struct directory_info d;

    memset(component, 0, i);
    i = 0;

    while(*name && *name != '/') {
      component[i] = *name;
      ++i;
      ++name;
    }

    if (!*component) {
      ++name;
      continue;
    }

    /*
      Search for the file in the directory.
    */
    for (size_t j = 0; j < (f->ext.size / sizeof(struct directory_info)); ++j) {
      struct filesystem_addr addr;
      struct buffer_info* b;

      addr = file_offset_to_addr(f, j * sizeof(struct directory_info));
      b = buffer_get(addr.num);
      d = *((struct directory_info*)b->data + j);
      buffer_put(b);

      if (strcmp(d.name, component) == 0) {
        found = 1;
        break;
      }
    }

    if (found) {
      file_put(f);
      f = file_get(d.num);
    }
    else {
      return NULL;
    }
  }

  return f;
}

/*
  file_get returns internal file information from an external file information
  number "file_info_num".
*/
struct file_info_int* file_get(uint32_t file_info_num) {
  struct file_info_int* ret = NULL;
  struct buffer_info* buffer_info = NULL;
  struct filesystem_addr addr = file_to_addr(file_info_num);

  buffer_info = buffer_get(addr.num);
  ret = file_pop(&free_file_infos);

  if (!buffer_info || !ret) {
    return NULL;
  }

  ret->ext = *(struct file_info_ext*)(buffer_info->data + addr.offset);
  return ret;
}

/*
  file_put writes the external file information from the internal file
  information "file_info" to the SD card.
*/
void file_put(const struct file_info_int* file_info) {
  struct buffer_info* buffer_info = NULL;
  struct filesystem_addr addr = file_to_addr(file_info->ext.num);

  buffer_info = buffer_get(addr.num);
  *(struct file_info_ext*)(buffer_info->data + addr.offset) = file_info->ext;
  buffer_put(buffer_info);
}

/*
  file_push adds an element "file_info" to the free file information list
  "list".
*/
void file_push(struct file_info_int* list, struct file_info_int* file_info) {
  file_info->next = list->next;
  file_info->prev = list;
  list->next->prev = file_info;
  list->next = file_info;
}

/*
  file_pop removes the first element of the free file information list "list"
  and returns it.
*/
struct file_info_int* file_pop(struct file_info_int* list) {
  struct file_info_int* ret = list->next;

  if (ret == list) {
    return NULL;
  }

  list->next = ret->next;
  ret->next->prev = list;
  return ret;
}

/*
  file_alloc allocates a struct file_info_int using the free file file
  information list and returns it.
*/
struct file_info_int* file_alloc() {
  uint32_t num;
  struct file_info_int* ret;

  /*
    If the free file information list is empty, then we search the filesystem
    to replenish it.
  */
  if (!filesystem_info.free_file_infos_size) {
    struct buffer_info* buffer_info;
    struct file_info_ext* file_info_exts;
    int is_full = 0;

    /*
      The file information blocks begin after the filesystem information block.
    */
    for (size_t i = 1; i < filesystem_info.file_infos_size; ++i) {
      buffer_info = buffer_get(i);
      file_info_exts = (struct file_info_ext*)buffer_info->data;

      for (size_t j = 0; j < FILE_INFO_PER_BLOCK; ++j) {
        if (filesystem_info.free_file_infos_size == FILESYSTEM_INFO_CACHE_SIZE) {
          is_full = 1;
          break;
        }

        /* External file information is free if its type is zero. */
        if (!file_info_exts[j].type) {
          filesystem_info.free_file_infos[filesystem_info.free_file_infos_size] = file_info_exts[j].num;
          ++filesystem_info.free_file_infos_size;
        }
      }

      buffer_put(buffer_info);

      if (is_full) {
        break;
      }
    }
  }

  --filesystem_info.free_file_infos_size;
  num = filesystem_info.free_file_infos[filesystem_info.free_file_infos_size];
  ret = file_get(num);
  ret->ext.type = 1;
  file_put(ret);
  return ret;
}

/*
  file_free frees the internal file information specified by "file_info". It
  tries to push the internal file information to the free file information
  list.
*/
void file_free(const struct file_info_int* file_info) {
  if (filesystem_info.free_file_infos_size == FILESYSTEM_INFO_CACHE_SIZE) {
    return;
  }

  filesystem_info.free_file_infos[filesystem_info.free_file_infos_size] = file_info->ext.num;
  ++filesystem_info.free_file_infos_size;
}

/*
  block_alloc allocates a block using the free block list and returns it.
*/
struct buffer_info* block_alloc() {
  uint32_t num;
  struct buffer_info* ret;

  --filesystem_info.free_blocks_size;
  num = filesystem_info.free_blocks[filesystem_info.free_blocks_size];
  ret = buffer_get(num);

  /*
    If we just read the last block from the free block list, then we replenish
    the free block list.
  */
  if (!filesystem_info.free_blocks_size) {
    filesystem_info.free_blocks_size = *((uint32_t*)ret->data);
    memcpy(filesystem_info.free_blocks, (uint32_t*)ret->data + 1, filesystem_info.free_blocks_size * sizeof(uint32_t));
  }

  memset(ret->data, 0, FILE_BLOCK_SIZE);
  return ret;
}

/*
  block_free frees the block specified by "buffer_info". It tries to push the
  block to the free block list.
*/
void block_free(struct buffer_info* buffer_info) {
  /*
    If the free block list is full and we can't push to it, then we write it to
    disk and point to it.
  */
  if (filesystem_info.free_blocks_size + 1 > FILESYSTEM_INFO_CACHE_SIZE) {
    *((uint32_t*)buffer_info->data) = filesystem_info.free_blocks_size;
    memcpy((uint32_t*)buffer_info->data + 1, filesystem_info.free_blocks, filesystem_info.free_blocks_size * sizeof(uint32_t));
    filesystem_info.free_blocks_size = 1;
    *filesystem_info.free_blocks = buffer_info->num;
    buffer_put(buffer_info);
  }
  else {
    filesystem_info.free_blocks[filesystem_info.free_blocks_size] = buffer_info->num;
    ++filesystem_info.free_blocks_size;
  }
}

/*
  file_to_addr returns the filesystem address specified by the external file
  information number "file_info_num".
*/
struct filesystem_addr file_to_addr(uint32_t file_info_num) {
  struct filesystem_addr ret;

  ret.num = file_num_to_block_num(file_info_num);
  ret.offset = file_num_to_block_offset(file_info_num);
  return ret;
}
