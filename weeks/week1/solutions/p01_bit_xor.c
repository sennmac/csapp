#include <stdint.h>

/*
 * 题目: 实现 bitXor(x, y)，仅使用 ~ 和 &。
 * 思路: 利用德摩根律，x^y = ~(~x & ~y) & ~(x & y)。
 * 正确性: 两项分别表示“至少一个为1”和“不能同时为1”，交集即异或。
 */
int bitXor(int x, int y) {
  return ~(~x & ~y) & ~(x & y);
}
