#include <kernel/device.h>
#include <drivers/terminal.h>

struct device* character_device_table[DEVICE_TABLE_SIZE];
struct device* block_device_table[DEVICE_TABLE_SIZE];

/*
  devices_init initializes devices and creates their respective special files.
  Currently the devices created are just hard-coded.
*/
void devices_init() {
  device_register(&terminal_device);
}

/*
  device_register registers the device "dev" in either the character or block
  device table.
*/
int device_register(struct device* dev) {
  struct device** table;

  if (dev->type == DT_CHARACTER) {
    table = character_device_table;
  }
  else {
    table = block_device_table;
  }

  table[dev->major] = dev;

  if (device_add(dev) < 0) {
    table[dev->major] = NULL;

    return -1;
  }

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
