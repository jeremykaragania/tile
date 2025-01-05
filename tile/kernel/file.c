#include <kernel/file.h>

const char* current_directory = ".";
const char* parent_directory = "..";

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
    Initialize the file table of the kernel's init process.
  */
  current_process()->file_tab = file_table_alloc();

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
  file_open opens a file specified by its name "name" and the access modes
  specified by "flags". On success, it returns a positive file descriptor to
  the file.
*/
int file_open(const char* name, int flags) {
  struct process_info* proc;
  struct file_info_int* file;
  struct file_table_entry* file_tab;
  int ret;

  file = name_to_file(name);

  if (!file) {
    return -1;
  }

  proc = current_process();

  /*
    Search for a free file descriptor in the file table.
  */
  for (size_t i = 3; i < FILE_TABLE_SIZE; ++i) {
    if (!proc->file_tab[i].file_int) {
      ret = i;
      break;
    }

    if (i == FILE_TABLE_SIZE - 1) {
      return -1;
    }
  }

  file_tab = &proc->file_tab[ret];
  file_tab->file_int = file;
  file_tab->status = flags;

  return ret;
}

/*
  file_close closes the file specified by the file descriptor "fd". On sucess 1
  is returned, and on failure 0 is returned.
*/
int file_close(int fd) {
  struct process_info* proc;

  if (fd < 1 || fd >= FILE_TABLE_SIZE) {
    return 0;
  }

  proc = current_process();
  proc->file_tab[fd].file_int = NULL;

  return 1;
}

/*
  file_resize resizes the file specified by "file" until it is at least "size"
  bytes.
*/
void file_resize(struct file_info_int* file, size_t size) {
  size_t curr_blocks = blocks_in_file(file->ext.size);
  size_t next_blocks = blocks_in_file(size);
  int delta = next_blocks - curr_blocks;
  int sign = delta > 0 ? 1 : -1;

  if (sign > 0) {
    file_push_blocks(file, abs(delta));
  }
  else {
    file_pop_blocks(file, abs(delta));
  }

  file->ext.size = size;
}

/*
  file_push_blocks pushes "count" blocks to the file "file".
*/
void file_push_blocks(struct file_info_int* file, size_t count) {
  size_t curr_blocks = blocks_in_file(file->ext.size);
  size_t offset;
  struct block_info block_info;
  uint32_t block_num;
  struct buffer_info* alloc_buffer;
  struct buffer_info* get_buffer;

  for (size_t i = 0; i < count; ++i) {
    offset = FILE_BLOCK_SIZE * (curr_blocks + i);
    block_info = file_offset_to_block(offset);
    block_num = file->ext.blocks[block_info.index];

    /*
      If the previous block index is different from the current block index, or
      this is the first block then we allocate it.
    */
    if (file_offset_to_block(offset - FILE_BLOCK_SIZE).index != block_info.index || !block_info.index) {
      alloc_buffer = block_alloc();
      file->ext.blocks[block_info.index] = alloc_buffer->num;
      block_num = alloc_buffer->num;
      buffer_put(alloc_buffer);
    }

    for (size_t i = 0; i < block_info.level; ++i) {
      get_buffer = buffer_get(block_num);

      /*
        If the previous block number index is different from the current block
        index, then we allocate one. This is a hack which works due to how a
        block number index depends on the offset and the level.
      */
      if (block_num_index(block_info.level - i, offset - FILE_BLOCK_SIZE) != block_num_index(block_info.level - i, offset)) {
        alloc_buffer = block_alloc();
        ((uint32_t*)(get_buffer->data))[block_num_index(block_info.level - i, offset)] = alloc_buffer->num;
        buffer_put(alloc_buffer);
      }

      block_num = ((uint32_t*)(get_buffer->data))[block_num_index(block_info.level - i, offset)];
      buffer_put(get_buffer);
    }
  }
}

/*
  file_pop_blocks pops "count" blocks from the file "file".
*/
void file_pop_blocks(struct file_info_int* file, size_t count) {
  size_t curr_blocks = blocks_in_file(file->ext.size);
  size_t offset;
  struct block_info block_info;
  uint32_t block_num;
  struct buffer_info* get_buffers[2];

  for (size_t i = 0; i < count; ++i) {
    offset = FILE_BLOCK_SIZE * (curr_blocks - (i + 1));
    block_info = file_offset_to_block(offset);
    block_num = file->ext.blocks[block_info.index];

    for (size_t i = 0; i < block_info.level; ++i) {
      get_buffers[0] = buffer_get(block_num);

      /*
        If the previous block number index is different from the current block
        index, then we allocate one. This is a hack which works due to how a
        block number index depends on the offset and the level.
      */
      if (block_num_index(block_info.level - i, offset - FILE_BLOCK_SIZE) != block_num_index(block_info.level - i, offset)) {
        get_buffers[1] = buffer_get(((uint32_t*)(get_buffers[0]->data))[block_num_index(block_info.level - i, offset)]);
        block_free(get_buffers[1]);
        buffer_put(get_buffers[1]);
      }

      block_num = ((uint32_t*)(get_buffers[0]->data))[block_num_index(block_info.level - i, offset)];
      buffer_put(get_buffers[0]);
    }

    /*
      If the next block index is different from the current block index, or
      this is the first block then we free it.
    */
    if (file_offset_to_block(offset - FILE_BLOCK_SIZE).index != block_info.index || !block_info.index) {
      get_buffers[0] = buffer_get(file->ext.blocks[block_info.index]);
      block_free(get_buffers[0]);
      buffer_put(get_buffers[1]);
    }
  }
}

/*
  file_offset_to_addr returns the filesystem address at the byte offset
  "offset" in the file represented by the file information "info".
*/
struct filesystem_addr file_offset_to_addr(const struct file_info_int* file_info, uint32_t offset) {
  struct filesystem_addr ret = {0, 0};
  struct buffer_info* buffer_info;
  struct block_info block_info;

  if (offset > L3_BLOCKS_END) {
    return ret;
  }

  block_info = file_offset_to_block(offset);
  ret.num = file_info->ext.blocks[block_info.index];
  ret.offset = offset % 512;

  if (block_info.level) {
    for (size_t i = 0; i < block_info.level; ++i) {
      buffer_info = buffer_get(ret.num);
      ret.num = ((uint32_t*)buffer_info->data)[block_num_index(block_info.level - i, offset)];
      buffer_put(buffer_info);
    }
  }

  return ret;
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
    ret.index = offset / FILE_BLOCK_SIZE;
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
  uint32_t begin;
  uint32_t step;

  /*
    At each block level, the beginning file byte offset along with how many
    bytes a block represents.
  */
  switch (level) {
    case 1:
      begin = L0_BLOCKS_END + 1;
      step = FILE_BLOCK_SIZE;
      break;
    case 2:
      begin = L1_BLOCKS_END + 1;
      step = L2_BLOCKS_COUNT * FILE_BLOCK_SIZE;
      break;
    case 3:
      begin = L2_BLOCKS_END + 1;
      step = L3_BLOCKS_COUNT * FILE_BLOCK_SIZE;
      break;
    default:
      return 0;
  }

  return (offset - begin) / step % 128;
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
  name_to_parent returns the parent name of the file name "name" in the buffer
  pointed to by "parent" which must be the same size as "name".
*/
char* name_to_parent(const char* name, char* parent) {
  size_t len = strlen(name);
  size_t i = 0;

  memcpy(parent, (void*)name, len);

  if (strlen(name) == 1 && *name == '/') {
    return parent;
  }

  while (name[len - i - 1] == '/') {
    ++i;
  }

  while (i < len) {
    if (name[len - i - 1] == '/') {
      while (name[len - i - 1] == '/') {
        ++i;
      }

      parent[len - i + 1] = 0;
      return parent;
    }

    ++i;
  }

  memcpy(parent, (void*)current_directory, len);

  return parent;
}

/*
  file_get returns internal file information from an external file information
  number "file_info_num". It is similar to buffer_get except we don't wait for
  free internal file information.
*/
struct file_info_int* file_get(uint32_t file_info_num) {
  struct file_info_int* curr = file_infos.next;
  struct filesystem_addr addr = file_to_addr(file_info_num);
  struct buffer_info* buffer_info = buffer_get(addr.num);

  if (!buffer_info) {
    return NULL;
  }

  while (curr != &file_infos) {
    if (curr->ext.num == file_info_num) {
      return curr;
    }

    curr = curr->next;
  }

  curr = file_pop(&free_file_infos);
  curr->ext = *(struct file_info_ext*)(buffer_info->data + addr.offset);
  file_push(&file_infos, curr);
  return curr;
}

/*
  file_put writes the external file information from the internal file
  information "file_info" to the SD card.
*/
void file_put(struct file_info_int* file_info) {
  struct filesystem_addr addr = file_to_addr(file_info->ext.num);
  struct buffer_info* buffer = buffer_get(addr.num);

  memcpy(buffer->data + addr.offset, &file_info->ext, sizeof(struct file_info_ext));
  buffer_put(buffer);
  file_remove(&file_infos, file_info);
  file_push(&free_file_infos, file_info);
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
  file_remove removes the internal file information "file_info" from the
  internal file information list "list".
*/
void file_remove(struct file_info_int* list, struct file_info_int* file_info) {
  if (list == file_info || list->next == list) {
    return;
  }

  file_info->next->prev = file_info->prev;
  file_info->prev->next = file_info->next;
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

/*
  file_table_alloc allocates a file table and returns a pointer to it.
*/
struct file_table_entry* file_table_alloc() {
  return memory_alloc(sizeof(struct file_table_entry) * FILE_TABLE_SIZE);
}
