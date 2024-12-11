.global get_counter
/*
  get_counter returns the contents of the physical count register.
*/
get_counter:
  mrrc p15, 0, r0, r1, c14
  bx lr
