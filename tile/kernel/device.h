#ifndef DEVICE_H
#define DEVICE_H

#include <kernel/list.h>
#include <kernel/file.h>

#define DEVICE_TABLE_SIZE 255
#define DEVICE_NAME_SIZE FILE_NAME_SIZE

/*
  enum device_type represents the type of a device.
*/
enum device_type {
  DT_CHARACTER,
  DT_BLOCK
};

/*
  struct device represents either a character or block device;
*/
struct device {
  char name[DEVICE_NAME_SIZE];
  struct file_operations* ops;
  unsigned int major;
  unsigned int minor;
  int type;
};

extern struct device* character_device_table[DEVICE_TABLE_SIZE];
extern struct device* block_device_table[DEVICE_TABLE_SIZE];

void devices_init();
int device_register(struct device* dev);
int device_add(const struct device* dev);

#endif
