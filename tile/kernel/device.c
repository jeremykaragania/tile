#include <kernel/device.h>

struct device* character_device_table[DEVICE_TABLE_SIZE];
struct device* block_device_table[DEVICE_TABLE_SIZE];
struct list_link devices_head = LIST_INIT(devices_head);

/*
  devices_init exposes all currently registered devices under "/dev". Can only
  be called after the filesystem is initialized.
*/
void devices_init() {
  struct list_link* curr = devices_head.next;
  struct device* dev;

  while (curr != &devices_head) {
    dev = list_data(curr, struct device, link);
    device_add(dev);

    curr = curr->next;
  }
}

/*
  device_register adds the device "dev" to the device list and registers it in
  either the character or block device table.
*/
int device_register(struct device* dev) {
  struct device** table;

  list_push(&devices_head, &dev->link);

  if (dev->type == DT_CHARACTER) {
    table = character_device_table;
  }
  else {
    table = block_device_table;
  }

  table[dev->major] = dev;

  return 0;
}

/*
  device_add creates a node for the device "dev" under "/dev".
*/
int device_add(const struct device* dev) {
  int type;

  if (file_chdir("/dev") < 0) {
    return -1;
  }

  if (dev->type == DT_CHARACTER) {
    type = FT_CHARACTER;
  }
  else {
    type = FT_BLOCK;
  }

  if (file_mknod(dev->name, type, make_dev(dev->major, dev->minor)) < 0) {
    return -1;
  }

  return 0;
}

/*
  file_to_device tries to return the file correlating to the file "file".
*/
struct device* file_to_device(struct file_info_int* file) {
  struct device** table;

  if (!is_file_device(file)) {
    return NULL;
  }

  if (file->ext.type == FT_CHARACTER) {
    table = character_device_table;
  }
  else {
    table = block_device_table;
  }

  return table[file->ext.major];
}
