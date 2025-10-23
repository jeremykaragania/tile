#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

extern uint32_t syscall_table[];

uint32_t get_syscall_number();
void undefined_syscall();

extern int do_syscall(uint32_t number);

#endif
