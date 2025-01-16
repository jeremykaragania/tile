#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>

extern void set_processor_mode(uint32_t mode);

/*
  struct processor_registers represents the core ARMv7-A processor registers.
*/
struct processor_registers {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t sp;
  uint32_t lr;
  uint32_t pc;
};

/*
  struct processor_mode represents the ARMv7-A processor modes.
*/
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

/*
  struct processor_info represents the information about an ARMv7-A processor.
*/
struct processor_info {
  struct processor_registers reg;
  int mode;
};

#endif
