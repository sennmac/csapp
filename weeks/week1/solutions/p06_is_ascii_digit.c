#include <stdint.h>

/*
 * 题目: 判断 x 是否在 ASCII '0'..'9' 范围内。
 * 思路: 把区间判断拆成两个“非负”判断:
 *   1) x - 0x30 >= 0
 *   2) 0x39 - x >= 0
 *   只要有一个为负，就不在区间内。
 *
 * 位级实现:
 *   lower = x + (~0x30 + 1)   // x-0x30
 *   upper = 0x39 + (~x + 1)   // 0x39-x
 *   负数的符号位为1，所以 (lower>>31) 或 (upper>>31) 只要有1就失败。
 *
 * 返回:
 *   !((lower >> 31) | (upper >> 31))
 *   两者都非负 -> 返回1；否则返回0。
 */
int isAsciiDigit(int x) {
  int lower = x + (~0x30 + 1);
  int upper = 0x39 + (~x + 1);
  return !((lower >> 31) | (upper >> 31));
}
