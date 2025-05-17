#include <drivers/sp804.h>

volatile struct dual_timer_registers* timer_0 = (struct dual_timer_registers*)TIMER_1_PADDR;
