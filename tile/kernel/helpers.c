#include <helpers.h>

int __aeabi_idiv(int numerator, int denominator) {
  int sign = 0;
  int q;
  if (denominator < 0) {
    sign ^= 1;
    denominator = -denominator;
  }
  if (numerator < 0) {
    sign ^= 1;
    numerator = -numerator;
  }
  q = __aeabi_uidiv(numerator, denominator);
  if (sign) {
    q = -q;
  }
  return q;
}

unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator) {
  int q = 0;
  while (numerator >= denominator) {
    numerator -= denominator;
    q += 1;
  }
  return q;
}
