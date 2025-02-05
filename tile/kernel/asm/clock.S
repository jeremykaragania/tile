#include <kernel/asm/clock.h>

.global init_clock
init_clock:
  ldr r0, =SYS_CLK_MHZ * 1000000
  mcr p15, 0, r0, c14, c0, 0
  bx lr

/*
  get_counter returns the contents of the physical count register.
*/
.global get_counter
get_counter:
  mrrc p15, 0, r0, r1, c14
  bx lr

/*
  get_timer returns the contents of the physical timer register.
*/
.global get_timer
get_timer:
  mrc p15, 0, r0, c14, c2, 0
  bx lr
