/*
  sp804.c provides an ARM Dual-Timer Module (SP804) driver.
*/

#include <drivers/sp804.h>

volatile struct dual_timer_registers* timer_0 = (struct dual_timer_registers*)TIMER_1_PADDR;

/*
  dual_timer_init initializes one of the dual timers.
*/
void dual_timer_init() {
  timer_0->timer1_load = 0xfffe;

  timer_0->timer1_control = 0;

  /* 32-bit counter. */
  timer_0->timer1_control |= 1 << 1;

  /* Timer module Interrupt enabled. */
  timer_0->timer1_control |= 1 << 5;

  /* Timer module is in periodic mode. */
  timer_0->timer1_control |= 1 << 6;

  /* Timer module enabled. */
  timer_0->timer1_control |= 1 << 7;
}
