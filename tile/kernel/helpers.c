#include <helpers.h>

struct divide_return divide(unsigned long long numerator, unsigned long long denominator) {
  struct divide_return ret = {
    0,
    numerator
  };
  while (ret.remainder >= denominator) {
    ret.remainder -= denominator;
    ret.quotient += 1;
  }
  return ret;
}

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
  q = divide(numerator, denominator).quotient;
  if (sign) {
    q = -q;
  }
  return q;
}

unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator) {
  return divide(numerator, denominator).quotient;
}

void __aeabi_idivmod(int numerator, int denominator) {
  struct divide_return ret = divide(numerator, denominator);
  value_in_regs((int)ret.quotient, (int)ret.remainder);
}

void __aeabi_uidivmod(unsigned int numerator, unsigned int denominator) {
  struct divide_return ret = divide(numerator, denominator);
  value_in_regs((unsigned int)ret.quotient, (unsigned int)ret.remainder);
}
