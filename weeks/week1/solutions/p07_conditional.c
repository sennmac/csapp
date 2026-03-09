#include <stdint.h>

/*
 * 题目: 实现 x ? y : z，不使用条件分支。
 * 思路: 先把 !!x 变成全1/全0掩码，再按位选择 y 或 z。
 * 正确性: mask 全1时保留 y；mask 全0时保留 z。
 */
int conditional(int x, int y, int z) {
  int mask = -!!x;
  return (mask & y) | (~mask & z);
}
