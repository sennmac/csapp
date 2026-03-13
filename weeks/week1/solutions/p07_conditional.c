#include <stdint.h>

/*
 * 题目: 实现 x ? y : z，不使用条件分支。
 * 思路: 先把 x 变成“全1/全0掩码”，再做按位选择。
 *
 * 关键变形:
 *   !!x  : x!=0 ->1, x==0 ->0
 *   -!!x : 1->0xFFFFFFFF, 0->0x00000000
 *   这样 mask 就能控制每一位来自 y 还是 z。
 *
 * 组合:
 *   (mask & y) | (~mask & z)
 *   mask=全1 -> 结果是 y
 *   mask=全0 -> 结果是 z
 *
 * 因此该表达式与 x ? y : z 完全等价。
 */
int conditional(int x, int y, int z) {
  int mask = -!!x;
  return (mask & y) | (~mask & z);
}
