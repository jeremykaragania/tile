#include <lib/syscall.h>

#define SYS_write 6

int main() {
  char buf[] = "Hello, World!\n";

  syscall(SYS_write, 1, buf, sizeof(buf));

  while(1);
}
