#include <stdint.h>

/*
 * 题目: 判断 x <= y。
 * 思路: 分“异号”和“同号”两类处理，避免直接做 y-x 时的溢出误判。
 *
 * 1) 异号(sign_diff=1):
 *    x<0 且 y>=0 时一定有 x<=y，结果就是 sx。
 *
 * 2) 同号(sign_diff=0):
 *    这时 y-x 不会因符号差而误判，检查 diff=y-x 是否非负即可。
 *    diff_neg=1 代表 diff<0，则 x>y；diff_neg=0 则 x<=y。
 *
 * 最终组合:
 *   (sign_diff & sx) | (!sign_diff & !diff_neg)
 *
 * 覆盖边界:
 *   - x=INT_MIN, y=INT_MAX（异号分支）
 *   - x=y（同号且 diff=0）
 *   - x>y / x<y（同号常规比较）
 */
int isLessOrEqual(int x, int y) {
  int sx = (x >> 31) & 1;
  int sy = (y >> 31) & 1;
  int sign_diff = sx ^ sy;
  int diff = y + (~x + 1);
  int diff_neg = (diff >> 31) & 1;
  return (sign_diff & sx) | (!sign_diff & !diff_neg);
}
