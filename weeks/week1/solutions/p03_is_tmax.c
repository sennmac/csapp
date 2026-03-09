#include <stdint.h>

/*
 * 题目: 判断 x 是否为补码最大值 Tmax。
 * 思路: Tmax + 1 得到 Tmin，二者按位相加后应为全1；同时排除 x=-1 的干扰。
 * 正确性: 仅当 x 为 Tmax 时，~(x + (x + 1)) == 0 且 (x + 1) != 0。
 */
int isTmax(int x) {
  int y = x + 1;
  return !~(x + y) & !!y;
}
