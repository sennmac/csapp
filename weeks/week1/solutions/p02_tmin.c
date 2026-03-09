#include <stdint.h>

/*
 * 题目: 返回 int32 的最小值 Tmin。
 * 思路: 最高位为1，其余位为0，即 0x80000000。
 * 正确性: 这正是补码最小值，十进制为 -2147483648。
 */
int tmin(void) {
  return (int)(1u << 31);
}
