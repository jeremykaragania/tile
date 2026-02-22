#include <lib/syscall.h>

#define SYS_read 5
#define SYS_write 6

#define BUF_SIZE 256

int main() {
  char buf[BUF_SIZE];
  int count;

  while (1) {
    count = syscall(SYS_read, 1, buf, sizeof(buf));

    syscall(SYS_write, 1, buf, count);
  }
}
