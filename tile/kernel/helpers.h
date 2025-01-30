#ifndef HELPERS_H
#define HELPERS_H

/*
  struct signed_divide_return represents the return result of a signed integer
  division.
*/
struct signed_divide_return {
  long long quotient;
  long long remainder;
};

/*
  struct unsigned_divide_return represents the return result of an unsigned
  integer division.
*/
struct unsigned_divide_return {
  unsigned long long quotient;
  unsigned long long remainder;
};

extern void value_in_regs();

static struct signed_divide_return signed_divide(long long numerator, long long denominator);
static struct unsigned_divide_return unsigned_divide(unsigned long long numerator, unsigned long long denominator);

int __aeabi_idiv(int numerator, int denominator);
unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator);

void __aeabi_idivmod(int numerator, int denominator);
void __aeabi_uidivmod(unsigned int numerator, unsigned int denominator);

void __aeabi_ldivmod(long long numerator, long long denominator);
void __aeabi_uldivmod(unsigned long long numerator, unsigned long long denominator);

#endif
