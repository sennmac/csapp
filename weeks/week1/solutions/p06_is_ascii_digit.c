#include <stdint.h>

/*
 * 题目: 判断 x 是否在 ASCII '0'..'9' 范围内。
 * 思路: 同时检查 x-0x30 >= 0 与 0x39-x >= 0。
 * 正确性: 两个条件都满足时，x 必定在闭区间 [0x30, 0x39]。
 */
int isAsciiDigit(int x) {
  int lower = x + (~0x30 + 1);
  int upper = 0x39 + (~x + 1);
  return !((lower >> 31) | (upper >> 31));
}
