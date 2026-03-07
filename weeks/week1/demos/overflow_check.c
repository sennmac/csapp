#include <stdio.h>
#include <stdint.h>
#include <limits.h>

static int add_ok_int32(int32_t x, int32_t y) {
  int32_t s = x + y;
  // overflow if x and y same sign but s different sign
  int32_t sx = x >> 31;
  int32_t sy = y >> 31;
  int32_t ss = s >> 31;
  return !((sx == sy) && (ss != sx));
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
    printf("x=%12d y=%12d sum=%12d add_ok=%d\n", x, y, s, add_ok_int32(x,y));
  }
  return 0;
}
