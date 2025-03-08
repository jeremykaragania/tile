/*
  helpers.c provides C library helpers functions for the compiler.

  The helpers implement operations which aren't implemented in hardware but
  have to be implemented in code.
*/

#include <kernel/helpers.h>

/*
  signed_divide_return returns the quotient and remainder of "numerator"
  divided by "denominator".
*/
static struct signed_divide_return signed_divide(long long numerator, long long denominator) {
  int sign = 0;
  struct unsigned_divide_return udr;
  struct signed_divide_return sdr;

  if (denominator < 0) {
    sign ^= 1;
    denominator = -denominator;
  }

  if (numerator < 0) {
    sign ^= 1;
    numerator = -numerator;
  }

  udr = unsigned_divide(numerator, denominator);
  sdr.quotient = udr.quotient;
  sdr.remainder = udr.remainder;

  if (sign) {
    sdr.quotient = -sdr.quotient;
  }

  return sdr;
}

/*
  unsigned_divide_return returns the quotient and remainder of "numerator"
  divided by "denominator".
*/
static struct unsigned_divide_return unsigned_divide(unsigned long long numerator, unsigned long long denominator) {
  struct unsigned_divide_return udr = {
    0,
    0
  };

  for (int i = 64; i >= 0; --i) {
    udr.remainder <<= 1;
    udr.remainder |= (numerator >> i) & 1;

    if (udr.remainder >= denominator) {
      udr.remainder -= denominator;
      udr.quotient |= 1ull << i;
    }
  }

  return udr;
}

/*
  __aeabi_idiv returns the quotient and remainder of "numerator" divided by
  "denominator".
*/
int __aeabi_idiv(int numerator, int denominator) {
  return signed_divide(numerator, denominator).quotient;
}

/*
  __aeabi_uidiv returns the quotient and remainder of "numerator" divided by
  "denominator".
*/
unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator) {
  return unsigned_divide(numerator, denominator).quotient;
}

/*
  __aeabi_idivmod returns the quotient and remainder of "numerator" divided by
  "denominator" in registers.
*/
void __aeabi_idivmod(int numerator, int denominator) {
  struct signed_divide_return idr = signed_divide(numerator, denominator);
  value_in_regs((int)idr.quotient, (int)idr.remainder);
}

/*
  __aeabi_uidivmod returns the quotient and remainder of "numerator" divided by
  "denominator" in registers.
*/
void __aeabi_uidivmod(unsigned int numerator, unsigned int denominator) {
  struct unsigned_divide_return udr = unsigned_divide(numerator, denominator);
  value_in_regs((unsigned int)udr.quotient, (unsigned int)udr.remainder);
}

/*
  __aeabi_ldivmod returns the quotient and remainder of "numerator" divided by
  "denominator" in registers.
*/
void __aeabi_ldivmod(long long numerator, long long denominator) {
  struct signed_divide_return sdr = signed_divide(numerator, denominator);
  value_in_regs((unsigned int)(sdr.quotient & 0xffffffffull), (unsigned int)(sdr.quotient >> 32), (unsigned int)(sdr.remainder >> 32), (unsigned int)(sdr.remainder & 0xffffffffull));
}

/*
  __aeabi_uldivmod returns the quotient and remainder of "numerator" divided by
  "denominator" in registers.
*/
void __aeabi_uldivmod(unsigned long long numerator, unsigned long long denominator) {
  struct unsigned_divide_return udr = unsigned_divide(numerator, denominator);
  value_in_regs((unsigned int)(udr.quotient & 0xffffffffull), (unsigned int)(udr.quotient >> 32), (udr.remainder & 0xffffffffull), (udr.remainder >> 32));
}
