#include <stdint.h>

/*
 * 题目: 判断 x 的所有奇数位(1,3,5,...)是否都为1。
 * 思路: 构造掩码 0xAAAAAAAA，仅保留奇数位后与掩码比较。
 * 正确性: (x & mask) == mask 等价于“每个奇数位都被置1”。
 */
int allOddBits(int x) {
  int mask = (int)0xAAAAAAAAu;
  return !((x & mask) ^ mask);
}
