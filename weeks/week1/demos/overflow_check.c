#include <stdio.h>
#include <stdint.h>
#include <limits.h>

// Original version: reason with sign bits directly.
static int add_ok_int32_orig(int32_t x, int32_t y) {
  int32_t s = x + y;
  int32_t sx = x >> 31;
  int32_t sy = y >> 31;
  int32_t ss = s >> 31;
  return !((sx == sy) && (ss != sx));
}

// Bitwise version: Data Lab style overflow test.
static int add_ok_int32_bitwise(int32_t x, int32_t y) {
  uint32_t ux = (uint32_t)x;
  uint32_t uy = (uint32_t)y;
  uint32_t us = ux + uy;
  // Overflow bit is 1 when x and y have same sign but sum has different sign.
  uint32_t overflow = ((~(ux ^ uy) & (ux ^ us)) >> 31) & 1u;
  return (int)(overflow ^ 1u);
}

int main(void) {
  int32_t tests[][2] = {
    {1, 2},
    {INT32_MAX, 0},
    {INT32_MAX, 1},
    {INT32_MIN, -1},
    {-100, -50},
  };

  for (int i = 0; i < (int)(sizeof(tests)/sizeof(tests[0])); i++) {
    int32_t x = tests[i][0], y = tests[i][1];
    int32_t s = x + y;
    int o1 = add_ok_int32_orig(x, y);
    int o2 = add_ok_int32_bitwise(x, y);
    printf("x=%12d y=%12d sum=%12d orig=%d bitwise=%d same=%d\n",
           x, y, s, o1, o2, o1 == o2);
  }
  return 0;
}
