#include <stdint.h>

/*
 * 题目: 给定 float 的位模式 uf，返回 2*f 的位模式。
 * 思路:
 *   1) NaN/Inf 直接返回；
 *   2) 非规格化: frac 左移，必要时进到规格化区；
 *   3) 规格化: exp + 1，若上溢变为 Inf。
 * 正确性: 按 IEEE754 单精度三类编码分别处理，覆盖边界行为。
 */
unsigned floatScale2(unsigned uf) {
  unsigned sign = uf & 0x80000000u;
  unsigned exp = uf & 0x7F800000u;
  unsigned frac = uf & 0x007FFFFFu;

  if (exp == 0x7F800000u) {
    return uf;
  }

  if (exp == 0) {
    frac <<= 1;
    if (frac & 0x00800000u) {
      exp = 0x00800000u;
      frac &= 0x007FFFFFu;
    }
    return sign | exp | frac;
  }

  exp += 0x00800000u;
  if (exp == 0x7F800000u) {
    frac = 0;
  }
  return sign | exp | frac;
}
