#include <stdint.h>

/*
 * 题目: 判断 x <= y。
 * 思路: 分符号与同符号两种情况:
 *   1) 符号不同时，x 为负则一定成立；
 *   2) 符号相同时，看 y-x 是否非负。
 * 正确性: 覆盖了补码比较中的全部分支且避免了跨符号减法误判。
 */
int isLessOrEqual(int x, int y) {
  int sx = (x >> 31) & 1;
  int sy = (y >> 31) & 1;
  int sign_diff = sx ^ sy;
  int diff = y + (~x + 1);
  int diff_neg = (diff >> 31) & 1;
  return (sign_diff & sx) | (!sign_diff & !diff_neg);
}
