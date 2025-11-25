/*
  file.c handles the filesystem.

  The filesystem is a UNIX-like one and the structures it uses are analogous.
  There are three structures in a filesystem: super blocks, file information
  structures, and data blocks.

  The filesystem is split into blocks of size BLOCK_SIZE. Each block is
  represented by its block number. A filesystem address is specified by a block
  number and the offset within that block.

  There is a single super block per filesystem represented by struct filesystem
  info. The super block is the first block of a filesystem and contains all the
  necessary information about the filesystem.

  There are two intimately related file information structures simply called
  external file information and internal file information which both store
  information about files. External file information is stored in secondary
  memory, while internal file information is stored in primary memory. When a
  file is read, its external file information is copied to an internal file
  information structure.

  File blocks store the underlying data of files. A file's data is accessed via
  up to four levels of indirection depending on its offset.
*/

#include <kernel/file.h>
#include <kernel/buffer.h>
#include <kernel/list.h>
#include <kernel/memory.h>
#include <kernel/page.h>
#include <kernel/process.h>
#include <lib/string.h>
#include <stdlib.h>

const char* current_directory = ".";
const char* parent_directory = "..";

struct filesystem_info filesystem_info;
struct file_info_int file_info_pool[FILE_INFO_CACHE_SIZE];

struct list_link files_head;
struct list_link devices_head;

/*
  filesystem_init initializes the filesystem and the relevant structures used
  by the kernel. It assumes that the filesystem begins at the beginning address
  of the SD card.
*/
void filesystem_init() {
  struct buffer_info* buffer_info = NULL;

  /*
    Initialize the file table of the kernel's init process.
  */
  current->file_tab = file_table_alloc();

  /*
    The first block contains information about the filesystem. The memory
    layout is specified by struct filesystem_info.
  */
  buffer_info = buffer_get(0);
  filesystem_info = *(struct filesystem_info*)buffer_info->data;

  /*
    Initialize the file information list.
  */
  list_init(&files_head);
}

/*
  filesystem_put writes all cached filesystem structures back to the
  filesystem.
*/
void filesystem_put() {
  struct list_link* curr;
  struct list_link* next;
  struct file_info_int* file;
  struct buffer_info* buffer;

  curr = files_head.next;

  while (curr != &files_head) {
    next = curr->next;
    file = list_data(curr, struct file_info_int, link);

    file_put(file);
    curr = next;
  }

  /*
    We don't forget to write the filesystem information back to its buffer.
  */
  buffer = buffer_get(0);
  *(struct filesystem_info*)buffer->data = filesystem_info;

  curr = buffers_head.next;

  while (curr != &buffers_head) {
    next = curr->next;
    buffer = list_data(curr, struct buffer_info, link);

    buffer_put(buffer);
    curr = next;
  }
}

/*
  is_file_owner checks if the user "user" is the owner of the file "file".
*/
bool is_file_owner(int user, struct file_info_int* file) {
  return user == 0 || file->ext.owner.user == user;
}

/*
  is_file_operation_allowed checks if the user "user" can perform the operation
  "operation" on the file "file". It returns 1 if the operation is allowed, and
  0 if it is not allowed.
*/
bool is_file_operation_allowed(int user, int operation, struct file_info_int* file) {
  uint32_t access = file->ext.access;
  struct file_owner owner = file->ext.owner;

  if (user == 0) {
    return true;
  }

  if (operation & R_OK) {
    if (!((user == owner.user && access & FA_READ_OWNER) || access & FA_READ_OTHERS)) {
      return false;
    }
  }

  if (operation & W_OK) {
    if (!((user == owner.user && access & FA_WRITE_OWNER) || access & FA_WRITE_OTHERS)) {
      return false;
    }
  }

  if (operation & X_OK) {
    if (!((user == owner.user && access & FA_EXEC_OWNER) || access & FA_EXEC_OTHERS)) {
      return false;
    }
  }

  return true;
}

/*
  open_flag_to_file_operation converts a file open flag to a file operation
  flag. It returns -1 on failure.
*/
int open_flag_to_file_operation(int flags) {
  if (flags & O_RDONLY) {
    return R_OK;
  }

  if (flags & O_RDWR) {
    return R_OK | W_OK;
  }

  if (flags & O_WRONLY) {
    return W_OK;
  }

  return -1;
}

/*
  file_open opens a file specified by its name "name" and the access modes
  specified by "flags". On success, it returns a positive file descriptor to
  the file.
*/
int file_open(const char* name, int flags) {
  struct file_info_int* file;
  struct file_table_entry* file_tab;
  int ret;
  int status = 0;
  int operation = open_flag_to_file_operation(flags);

  if (operation < 0) {
    return -1;
  }

  file = name_to_file(name);

  if (!file) {
    return -1;
  }

  if (!is_file_operation_allowed(current->euid, operation, file)) {
    return -1;
  }

  /*
    Convert the flags to a the file's status.
  */
  if (flags & O_RDONLY) {
    status |= FS_READ;
  }

  if (flags & O_WRONLY) {
    status |= FS_WRITE;
  }

  if (!status) {
    return -1;
  }

  ret = get_file_descriptor(current->file_tab);

  file_tab = &current->file_tab[ret];
  file_tab->status = status;
  file_tab->offset = 0;
  file_tab->file_int = file;

  return ret;
}

/*
  file_read tries to read up to "count" bytes from the file specified by the
  file descriptor "fd" into the buffer "buf". It returns the number of bytes
  read.
*/
int file_read(int fd, void* buf, size_t count) {
  struct file_table_entry* file_tab;
  struct file_info_int* file;
  struct filesystem_addr addr;
  struct buffer_info* buffer;
  int ret = 0;

  file_tab = &current->file_tab[fd];
  file = file_tab->file_int;

  if (!(file_tab->status & FS_READ)) {
    return -1;
  }

  /*
    We cap "count" at the file's size.
  */
  if (count > file->ext.size) {
    count = file->ext.size;
  }

  /*
    Read as many blocks as we can without exceeding "count".
  */
  for (size_t i = 0; i < count / BLOCK_SIZE; ++i) {
    addr = file_offset_to_addr(file, i * BLOCK_SIZE + current->file_tab[fd].offset);
    buffer = buffer_get(addr.num);
    memcpy((char*)buf + ret, buffer->data + addr.offset, BLOCK_SIZE);
    buffer_put(buffer);
    ret += BLOCK_SIZE;
  }

  /*
    Read the remaining bytes.
  */
  addr = file_offset_to_addr(file, ret + current->file_tab[fd].offset);
  buffer = buffer_get(addr.num);
  memcpy((char*)buf + ret, buffer->data + addr.offset, count % BLOCK_SIZE);
  buffer_put(buffer);
  ret += count % BLOCK_SIZE;

  current->file_tab[fd].offset += ret;

  return ret;
}

/*
  file_write tries to write up to "count" bytes from the buffer "buf" into the
  file specified by the file descriptor "fd". It returns the number of bytes
  written.
*/
int file_write(int fd, const void* buf, size_t count) {
  struct file_table_entry* file_tab;
  struct file_info_int* file;
  struct filesystem_addr addr;
  struct buffer_info* buffer;
  int ret = 0;

  file_tab = &current->file_tab[fd];
  file = file_tab->file_int;

  if (!(file_tab->status & FS_WRITE)) {
    return -1;
  }

  file_resize(file, file_tab->offset + count);

  /*
    Write as many blocks as we can without exceeding "count".
  */
  for (size_t i = 0; i < count / BLOCK_SIZE; ++i) {
    addr = file_offset_to_addr(file, i * BLOCK_SIZE + file_tab->offset);
    buffer = buffer_get(addr.num);
    memcpy(buffer->data + addr.offset, (char*)buf + ret, BLOCK_SIZE);
    buffer_put(buffer);
    ret += BLOCK_SIZE;
  }

  /*
    Write the remaining bytes.
  */
  addr = file_offset_to_addr(file, ret + file_tab->offset);
  buffer = buffer_get(addr.num);
  memcpy(buffer->data + addr.offset, (char*)buf + ret, count % BLOCK_SIZE);
  buffer_put(buffer);
  ret += count % BLOCK_SIZE;

  file_tab->offset += ret;

  return ret;
}

/*
  file_close closes the file specified by the file descriptor "fd". On success 0
  is returned, and on failure -1 is returned.
*/
int file_close(int fd) {
  if (fd < 1 || fd >= FILE_TABLE_SIZE) {
    return -1;
  }

  current->file_tab[fd].file_int = NULL;

  return 0;
}

/*
  file_mkdnod tries to create a node specified by "pathname" of type "type". It
  returns 0 on success, and -1 on failure.
*/
int file_mknod(const char* pathname, int type) {
  struct file_info_int* file = name_to_file(pathname);
  char* parent_name;
  char* file_name;
  struct file_info_int* parent;
  size_t parent_size;
  struct filesystem_addr addr;
  struct directory_info directory;
  struct buffer_info* buffer;

  if (file) {
    return -1;
  }

  parent_name = memory_alloc(strlen(pathname));
  file_name = memory_alloc(strlen(pathname));
  get_pathname_info(pathname, parent_name, file_name);

  parent = name_to_file(parent_name);

  /*
    A file can only be created in a directory.
  */
  if (!parent || parent->ext.type != FT_DIRECTORY) {
    return -1;
  }

  parent_size = parent->ext.size;

  /*
    Allocate a new file and allocate enough space in the parent block for a
    directory entry for the new file.
  */
  file = file_alloc();
  file->ext.type = type;
  file_resize(parent, parent_size + sizeof(struct directory_info));

  addr = file_offset_to_addr(parent, parent_size);

  directory.num = file->ext.num;
  memcpy(directory.name, file_name, strlen(pathname) + 1);

  buffer = buffer_get(addr.num);
  memcpy(buffer->data + addr.offset, &directory, sizeof(directory));

  buffer_put(buffer);

  return 0;
}

/*
  file_creat creates a file specified by "pathname" with the flags "flags" and
  returns a file descriptor to it.
*/
int file_creat(const char* pathname, int flags) {
  struct file_info_int* file;
  struct file_table_entry* file_tab;
  int ret;

  if (!file_mknod(pathname, FT_REGULAR)) {
    return -1;
  }

  file = name_to_file(pathname);
  ret = get_file_descriptor(current->file_tab);

  file_tab = &current->file_tab[ret];
  file_tab->status = flags;
  file_tab->offset = 0;
  file_tab->file_int = file;

  return ret;
}

/*
  file_seek sets the offset of a file descriptor "fd" to "offset".
*/
int file_seek(int fd, size_t offset) {
  size_t size = current->file_tab[fd].file_int->ext.size;

  if (offset > size) {
    return size;
  }

  current->file_tab[fd].offset = offset;

  return offset;
}

/*
  file_access checks the calling process's permissions to access the file
  specified by "file".
*/
int file_access(char* pathname, int mode) {
  struct file_info_int* file;

  file = name_to_file(pathname);

  if (!file || !is_file_operation_allowed(current->euid, mode, file)) {
    return -1;
  }

  return 0;
}

/*
  file_chmod changes the access permissions of the file specified by "pathname"
  to the mode "mode".
*/
int file_chmod(char* pathname, int mode) {
  struct file_info_int* file;

  file = name_to_file(pathname);

  if (!file || !is_file_owner(current->euid, file)) {
    return -1;
  }

  file->ext.access = mode;

  return 0;
}

/*
  file_chown changes the owner and group of the file specified by "pathname" to
  "owner" and "group" respectively.
*/
int file_chown(char* pathname, int owner, int group) {
  struct file_info_int* file;

  file = name_to_file(pathname);

  if (!file || !is_file_owner(current->euid, file)) {
    return -1;
  }

  file->ext.owner.user = owner;
  file->ext.owner.group = group;

  return 0;
}

/*
  file_map maps the file specified by the file descriptor "fd" with the flags
  "flags". On success it returns a pointer to the mapped area.
*/
void* file_map(int fd, int flags) {
  struct list_link* pages_head = &(current->mem->pages_head);
  struct file_info_int* file = current->file_tab[fd].file_int;
  struct page_region* region;
  void* addr;

  addr = find_unmapped_region(file->ext.size);

  if (!addr) {
    return NULL;
  }

  region = create_page_region(pages_head, (uint32_t)addr, page_count(file->ext.size), flags);

  if (!region) {
    return NULL;
  }

  region->file_int = file;

  return addr;
}

/*
  file_chdir changes the current directory to the directory specified by
  "pathname". It returns 0 on success, and -1 on failure.
*/
int file_chdir(const char* pathname) {
  struct file_info_int* file = name_to_file(pathname);

  if (!file || file->ext.type != FT_DIRECTORY) {
    return -1;
  }

  /*
    Release the old directory and set the current directory to the directory
    specified by "pathname".
  */
  file_put(file_get(current->file_num));
  current->file_num = file->ext.num;

  return 0;
}

/*
  get_file_descriptor searches for a free file descriptor in the file table
  "file_tab" and returns it on success, and -1 on failure.
*/
int get_file_descriptor(const struct file_table_entry* file_tab) {
  for (size_t i = 3; i < FILE_TABLE_SIZE; ++i) {
    if (!file_tab[i].file_int) {
      return i;
    }
  }

  return -1;
}

/*
  file_resize resizes the file specified by "file" until it is at least "size"
  bytes. It returns 0 on success and -1 on failure.
*/
int file_resize(struct file_info_int* file, size_t size) {
  size_t curr_blocks = blocks_in_file(file->ext.size);
  size_t next_blocks = blocks_in_file(size);
  int delta = next_blocks - curr_blocks;
  int sign = delta > 0 ? 1 : -1;

  if (size > MAX_FILE_SIZE) {
    return -1;
  }

  if (sign > 0) {
    file_push_blocks(file, abs(delta));
  }
  else {
    file_pop_blocks(file, abs(delta));
  }

  file->ext.size = size;
  file_put(file);

  return 0;
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
    offset = BLOCK_SIZE * (curr_blocks + i);
    block_info = file_offset_to_block(offset);
    block_num = file->ext.blocks[block_info.index];

    /*
      If the previous block index is different from the current block index, or
      this is the first block then we allocate it.
    */
    if (file_offset_to_block(offset - BLOCK_SIZE).index != block_info.index || !block_info.index) {
      alloc_buffer = block_alloc();
      file->ext.blocks[block_info.index] = alloc_buffer->num;
      block_num = alloc_buffer->num;
      buffer_put(alloc_buffer);
    }

    /*
      Starting from the first block level, we iterate up to the level of the
      current block, allocating intermediate blocks where neccessary.
    */
    for (size_t j = 0; j < block_info.level; ++j) {
      size_t prev_index = block_num_index(block_info.level - j, offset - BLOCK_SIZE);
      size_t curr_index = block_num_index(block_info.level - j, offset);

      get_buffer = buffer_get(block_num);

      /*
        If the previous block number index is different from the current block
        index, then we allocate one. This is a hack which works due to how a
        block number index depends on the offset and the level.
      */
      if (prev_index != curr_index) {
        alloc_buffer = block_alloc();
        ((uint32_t*)(get_buffer->data))[curr_index] = alloc_buffer->num;
        buffer_put(alloc_buffer);
      }

      block_num = ((uint32_t*)(get_buffer->data))[curr_index];
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
    offset = BLOCK_SIZE * (curr_blocks - (i + 1));
    block_info = file_offset_to_block(offset);
    block_num = file->ext.blocks[block_info.index];

    /*
      Starting from the first block level, we iterate up to the level of the
      current block, allocating intermediate blocks where neccessary.
    */
    for (size_t j = 0; j < block_info.level; ++j) {
      size_t prev_index = block_num_index(block_info.level - j, offset - BLOCK_SIZE);
      size_t curr_index = block_num_index(block_info.level - j, offset);

      get_buffers[0] = buffer_get(block_num);

      /*
        If the previous block number index is different from the current block
        index, then we allocate one. This is a hack which works due to how a
        block number index depends on the offset and the level.
      */
      if (prev_index != curr_index) {
        get_buffers[1] = buffer_get(((uint32_t*)(get_buffers[0]->data))[curr_index]);
        block_free(get_buffers[1]);
        buffer_put(get_buffers[1]);
      }

      block_num = ((uint32_t*)(get_buffers[0]->data))[curr_index];
      buffer_put(get_buffers[0]);
    }

    /*
      If the next block index is different from the current block index, or
      this is the first block then we free it.
    */
    if (file_offset_to_block(offset - BLOCK_SIZE).index != block_info.index || !block_info.index) {
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

  if (offset > file_info->ext.size) {
    return ret;
  }

  block_info = file_offset_to_block(offset);
  ret.num = file_info->ext.blocks[block_info.index];
  ret.offset = offset % BLOCK_SIZE;

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
  uint32_t begin;
  uint32_t step;

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
    num = current->file_num;
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

      if (!is_file_operation_allowed(current->euid, R_OK | X_OK, f)) {
        return NULL;
      }

      addr = file_offset_to_addr(f, j * sizeof(struct directory_info));
      b = buffer_get(addr.num);
      d = *((struct directory_info*)b->data + (j % DIRECTORIES_PER_BLOCK));
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
  get_pathname_info returns the parent and file name of the pathname "pathname"
  respectively in "parent" and "pathname". "parent" and "file" must be the same
  size as "pathname".
*/
void get_pathname_info(const char* pathname, char* parent, char* file) {
  size_t len = strlen(pathname);
  size_t i = 0;

  memcpy(parent, (void*)pathname, len);
  memcpy(file, (void*)pathname, len);

  if (strlen(pathname) == 1 && *pathname == '/') {
    return;
  }

  while (i < len) {
    if (pathname[len - i - 1] == '/') {
      parent[len - i] = 0;
      memcpy(file, pathname + len - i, i);
      file[i] = 0;
      return;
    }

    ++i;
  }

  memcpy(parent, (void*)current_directory, len);
}

/*
  normalize_pathname normalizes a path name "pathname" and returns it. It
  removes superfluous '/' characters.
*/
char* normalize_pathname(char* pathname) {
  size_t len = strlen(pathname);
  size_t i = 0;
  size_t j = 0;

  while (i < len) {
    if (pathname[i] == '/') {
      while (pathname[i] == '/') {
        ++i;
      }
      pathname[j] = '/';
      ++j;
    }

    pathname[j] = pathname[i];

    ++j;
    ++i;
  }

  pathname[j] = 0;

  return pathname;
}

/*
  file_get returns internal file information from an external file information
  number "file_info_num". It is similar to buffer_get except we don't wait for
  free internal file information.
*/
struct file_info_int* file_get(uint32_t file_info_num) {
  struct list_link* curr;
  struct filesystem_addr addr;
  struct file_info_int* file;
  struct buffer_info* buffer;

  curr = files_head.next;
  addr = file_to_addr(file_info_num);
  buffer = buffer_get(addr.num);

  if (!buffer) {
    return NULL;
  }

  while (curr != &files_head) {
    file = list_data(curr, struct file_info_int, link);

    if (file->ext.num == file_info_num) {
      return file;
    }

    curr = curr->next;
  }

  file = memory_alloc(sizeof(struct file_info_int));
  file->ext = *(struct file_info_ext*)(buffer->data + addr.offset);
  list_push(&files_head, &file->link);

  return file;
}


/*
  file_put writes the external file information from the internal file
  information "file_info" to the SD card.
*/
void file_put(struct file_info_int* file_info) {
  struct filesystem_addr addr;
  struct buffer_info* buffer;

  addr = file_to_addr(file_info->ext.num);
  buffer = buffer_get(addr.num);

  memcpy(buffer->data + addr.offset, &file_info->ext, sizeof(struct file_info_ext));
  buffer_put(buffer);
  list_remove(&files_head, &file_info->link);
  memory_free(file_info);
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
  uint32_t num = filesystem_info.free_blocks[filesystem_info.next_free_block];
  struct buffer_info* ret = buffer_get(num);

  /*
    If we just read the last block from the free block list, then we replenish
    the free block list.
  */
  if (!filesystem_info.next_free_block) {
    filesystem_info.free_blocks_size = *((uint32_t*)ret->data);
    filesystem_info.next_free_block = filesystem_info.free_blocks_size - 1;
    memcpy(filesystem_info.free_blocks, (uint32_t*)ret->data + 1, filesystem_info.free_blocks_size * sizeof(uint32_t));
  }
  else {
    --filesystem_info.next_free_block;
  }

  memset(ret->data, 0, BLOCK_SIZE);
  return ret;
}

/*
  block_free frees the block specified by "buffer_info". It tries to push the
  block to the free block list.
*/
void block_free(struct buffer_info* buffer_info) {
  size_t free_blocks_size = filesystem_info.next_free_block + 1;

  /*
    If the free block list is full and we can't push to it, then we write it to
    disk and point to it.
  */
  if (free_blocks_size == FILESYSTEM_INFO_CACHE_SIZE) {
    *((uint32_t*)buffer_info->data) = free_blocks_size;
    memcpy((uint32_t*)buffer_info->data + 1, filesystem_info.free_blocks, free_blocks_size * sizeof(uint32_t));
    filesystem_info.next_free_block = 0;
    *filesystem_info.free_blocks = buffer_info->num;
    buffer_put(buffer_info);
  }
  else {
    filesystem_info.free_blocks[free_blocks_size] = buffer_info->num;
    ++filesystem_info.next_free_block;
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
