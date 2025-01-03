#include <lib/string.h>

/*
  strlen returns the length of the string "s" excluding the null terminator.
*/
size_t strlen(const char* s) {
  size_t i = 0;

  while (*s++) {
    ++i;
  }

  return i;
}

/*
  strcmp compares the two strings "s1" and "s2". It returns 0 if they are
  equal, a negative value if "s1" is less than "s2", and a positive value if
  "s1" is greater than "s2".
*/
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

/*
  strncmp is the same as strcmp except it only compares "n" bytes of "s1" and
  "s2".
*/
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

/*
  meset sets "n" bytes in the buffer "s" to the byte "c".
*/
void* memset(void *s, int c, size_t n) {
  while (n--) {
    ((char*)s)[n] = c;
  }

  return s;
}

/*
  memcpy copies "n" bytes from the buffer "src" to "dest". The buffers must not
  overlap.
*/
void* memcpy(void *dest, const void *src, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    ((char*)dest)[i] = ((char*)src)[i];
  }

  return dest;
}

/*
  memmove copies "n" bytes from the buffer "src" to "dest". The buffers can
  overlap.
*/
void *memmove(void *dest, const void *src, size_t n) {
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
