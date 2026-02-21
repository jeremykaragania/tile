#include <lib/syscall.h>

#define BUF_SIZE 256

int main() {
  char buf[BUF_SIZE];
  int count;

  while (1) {
    count = syscall(5, 1, buf, sizeof(buf));

    syscall(6, 1, buf, count);
  }
}
