#include <drivers/sp804.h>

volatile struct dual_timer_registers* timer_0 = (struct dual_timer_registers*)TIMER_1_PADDR;

void dual_timer_init() {
  timer_0->timer1_control = 1 << 6;
  timer_0->timer1_control = 1 << 7;
}
