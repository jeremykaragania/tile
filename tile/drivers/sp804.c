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
  timer_0->timer1_control |= 1 << 1;
  timer_0->timer1_control |= 1 << 5;
  timer_0->timer1_control |= 1 << 6;
  timer_0->timer1_control |= 1 << 7;
}
