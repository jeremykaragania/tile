#include <lib/stdlib.h>

/*
  abs returns the absolute value of the integer "j".
*/
int abs(int j) {
  if (j < 0) {
    return -j;
  }

  return j;
}
