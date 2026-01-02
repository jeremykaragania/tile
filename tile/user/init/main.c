#include <lib/syscall.h>

int main() {
  char buf[] = "Hello, World!\n";

  syscall(6, 1, buf, sizeof(buf));

  while(1);
}
