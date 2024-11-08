#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>

extern void set_processor_mode(uint32_t mode);

enum processor_mode {
  PM_USR = 0x10,
  PM_FIQ,
  PM_IRQ,
  PM_SVC,
  PM_MON,
  PM_ABT,
  PM_HYP,
  PM_UND,
  PM_SYS
};

#endif
