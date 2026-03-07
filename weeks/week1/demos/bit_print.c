#include <stdio.h>
#include <stdint.h>

static void print_bits_u32(uint32_t x) {
  for (int i = 31; i >= 0; --i) {
    putchar(((x >> i) & 1u) ? '1' : '0');
    if (i % 4 == 0 && i != 0) putchar('_');
  }
}

int main(void) {
  int32_t vals[] = {0, 1, -1, 2, -2, 0x7fffffff, (int32_t)0x80000000u};
  int n = (int)(sizeof(vals) / sizeof(vals[0]));

  for (int i = 0; i < n; i++) {
    int32_t v = vals[i];
    printf("v=%11d  hex=0x%08x  bits=", v, (uint32_t)v);
    print_bits_u32((uint32_t)v);
    putchar('\n');
  }
  return 0;
}
