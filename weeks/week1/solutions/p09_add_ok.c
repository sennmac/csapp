#include <stdint.h>

/*
 * 题目: 判断 x + y 是否发生有符号溢出。
 * 思路: 只有“同号相加且结果异号”才溢出。
 * 正确性: 利用符号位关系实现判定，返回1表示安全、0表示溢出。
 */
int addOK(int x, int y) {
  int s = x + y;
  int sx = x >> 31;
  int sy = y >> 31;
  int ss = s >> 31;
  int overflow = (!(sx ^ sy)) & (sx ^ ss);
  return !overflow;
}
