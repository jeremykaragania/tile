#ifndef SP804_H
#define SP804_H

#include <kernel/asm/memory.h>
#include <stdint.h>

/*
  struct timer_registers represents the registers of the ARM Dual-Timer Module
  (SP804).
*/
struct dual_timer_registers {
  uint32_t timer1_load;
  const uint32_t timer1_value;
  uint32_t timer1_control;
  uint32_t timer1_int_clr;
  const uint32_t timer1_ris;
  const uint32_t timer1_mis;
  uint32_t timer1_bg_load;
  const uint32_t reserved_0;
  uint32_t timer2_load;
  const uint32_t timer2_value;
  uint32_t timer2_control;
  uint32_t timer2_int_clr;
  const uint32_t timer2_ris;
  const uint32_t timer2_mis;
  uint32_t timer2_bg_load;
  const uint32_t reserved_1[945];
  uint32_t timer1_tcr;
  uint32_t timer1_top;
  const uint32_t reserved_2[54];
  const uint32_t timer_periph_id[4];
  const uint32_t timer_pcell_id[4];
};

extern volatile struct dual_timer_registers* timer_0;

void dual_timer_init();

#endif
