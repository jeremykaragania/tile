#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define MAX_SYSCALL_NUMBER (sizeof(syscall_table) / sizeof(uint32_t) - 1)

extern uint32_t syscall_table[];

int do_syscall(uint32_t number);
uint32_t get_syscall_number();
int undefined_syscall();

extern int dispatch_syscall(uint32_t number);

#endif
