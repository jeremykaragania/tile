#ifndef HELPERS_H
#define HELPERS_H

struct divide_return {
  unsigned long long quotient;
  unsigned long long remainder;
};

struct divide_return divide(unsigned long long numerator, unsigned long long denominator);

int __aeabi_idiv(int numerator, int denominator);
unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator);

#endif
