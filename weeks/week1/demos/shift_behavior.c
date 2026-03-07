#include <stdio.h>
#include <stdint.h>

int main(void) {
  int32_t a = -4;
  uint32_t b = (uint32_t)a;

  printf("a = %d, (uint32)a = %u\n", a, b);
  for (int k = 0; k <= 4; k++) {
    int32_t ar = a >> k;
    uint32_t lr = b >> k;
    printf(">> %d : arith=%11d  hex=0x%08x | logical=%11u hex=0x%08x\n",
           k, ar, (uint32_t)ar, lr, lr);
  }
  return 0;
}
