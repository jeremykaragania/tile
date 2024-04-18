#include <string.h>

void* memcpy(void *dest, void *src, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i) {
    ((char*)dest)[i] = ((char*)src)[i];
  }
  return dest;
}

void *memmove(void *dest, void *src, size_t n) {
  if (dest <= src) {
    memcpy(dest, src, n);
  }
  else {
    size_t i;
    for (i = 0; i < n; ++i) {
      ((char*)dest)[n-i-1] = ((char*)src)[n-i-1];
    }
  }
  return dest;
}
