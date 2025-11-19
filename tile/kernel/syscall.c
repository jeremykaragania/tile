/*
 syscall.c handles system calls.

 The calling convention is similar to Linux. The "svc #0" instruction is used
 to switch to kernelspace, the "r7" register indicates the system call number,
 the "r0" and "r1" registers are used to return the system call results, and
 registers "r0" to "r6" are used to pass up to 7 arguments to the system call.
*/

#include <kernel/syscall.h>
#include <kernel/file.h>
#include <kernel/process.h>

/*
  The syscall table indexed by a syscall number.
*/
uint32_t syscall_table[] = {
  (uint32_t)file_access,
  (uint32_t)file_chmod,
  (uint32_t)file_chown,
  (uint32_t)file_open,
  (uint32_t)file_read,
  (uint32_t)file_write,
  (uint32_t)file_close,
  (uint32_t)file_mknod,
  (uint32_t)file_creat,
  (uint32_t)file_seek,
  (uint32_t)file_chdir
};

/*
  do_syscall does some initial checks before passing the system call number to
  the system call dispatcher.
*/
int do_syscall(uint32_t number) {
  int ret;

  if (number > MAX_SYSCALL_NUMBER) {
    return -1;
  }

  ret = dispatch_syscall(number);

  return ret;
}

/*
  get_syscall_number returns the syscall number of the current process.
*/
uint32_t get_syscall_number() {
  return current->reg.r7;
}

/*
  undefined_syscall handles an undefined syscall. Currently it just returns.
*/
int undefined_syscall() {
  return -1;
}
