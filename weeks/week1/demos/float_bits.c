#include <stdio.h>
#include <stdint.h>
#include <string.h>

static uint32_t f2u(float f) {
  uint32_t u;
  memcpy(&u, &f, sizeof(u));
  return u;
}

int main(void) {
  float vals[] = {0.0f, -0.0f, 1.0f, -1.0f, 0.5f, 2.0f, 3.1415926f, 1.0f/0.0f, 0.0f/0.0f, 8,9,10};
  int n = (int)(sizeof(vals)/sizeof(vals[0]));

  for (int i = 0; i < n; i++) {
    float f = vals[i];
    uint32_t u = f2u(f);
    uint32_t sign = (u >> 31) & 1u;
    uint32_t exp  = (u >> 23) & 0xffu;
    uint32_t frac = u & 0x7fffffu;

    printf("f=%12.6g  u=0x%08x  sign=%u exp=0x%02x frac=0x%06x\n",
           f, u, sign, exp, frac);
  }

  return 0;
}
