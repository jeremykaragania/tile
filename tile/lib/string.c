#include <lib/string.h>

size_t strlen(const char* s) {
  size_t i = 0;

  while (*s++) {
    ++i;
  }

  return i;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 || *s2) {
    if (*s1 != *s2) {
      if (*s1 < *s2) {
        return -1;
      }
      else {
        return 1;
      }
    }

    ++s1;
    ++s2;
  }

  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  size_t i = 0;

  while((*s1 || *s2) && i < n) {
    if (*s1 != *s2) {
      if (*s1 < *s2) {
        return -1;
      }
      else {
        return 1;
      }
    }

    ++s1;
    ++s2;
    ++i;
  }

  return 0;
}

void* memset(void *s, int c, size_t n) {
  while (n--) {
    ((char*)s)[n] = c;
  }

  return s;
}

void* memcpy(void *dest, void *src, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    ((char*)dest)[i] = ((char*)src)[i];
  }

  return dest;
}

void *memmove(void *dest, void *src, size_t n) {
  if (dest <= src) {
    memcpy(dest, src, n);
  }
  else {
    for (size_t i = 0; i < n; ++i) {
      ((char*)dest)[n-i-1] = ((char*)src)[n-i-1];
    }
  }

  return dest;
}
