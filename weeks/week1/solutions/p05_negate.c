#include <stdint.h>

/*
 * 题目: 计算 -x。
 * 思路: 补码取反加一。
 * 正确性: 对任意补码整数，~x + 1 与 -x 等价。
 */
int negate(int x) {
  return ~x + 1;
}
