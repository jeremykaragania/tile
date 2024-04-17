#include <string.h>

void* memcpy(void *dest, void *src, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i) {
    ((char*)dest)[i] = ((char*)src)[i];
  }
  return dest;
}
