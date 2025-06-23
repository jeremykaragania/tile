#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <stdint.h>

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
  uint32_t cpsr;
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

extern void enable_interrupts();
extern void disable_interrupts();
extern void init_stack_pointers();
extern void set_processor_mode(uint32_t mode);
extern void restore_registers(struct processor_registers* r);

#endif
