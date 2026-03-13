#include <stdint.h>

/*
 * 题目: 判断 x + y 是否发生有符号溢出。
 * 核心规律: 只有“同号相加且结果异号”才会溢出。
 *
 * 变量含义:
 *   sx, sy, ss 分别是 x/y/s 的符号位（0表示非负，-1表示负）
 *
 * 判定:
 *   !(sx ^ sy)   -> x 和 y 同号
 *   (sx ^ ss)    -> x 和 sum 异号
 *   overflow = (!(sx ^ sy)) & (sx ^ ss)
 *
 * 返回:
 *   !overflow
 *   1 表示加法安全，0 表示溢出。
 *
 * 示例:
 *   INT_MAX + 1  -> 溢出
 *   INT_MIN + (-1) -> 溢出
 *   1 + 2 -> 不溢出
 */
int addOK(int x, int y) {
  int s = x + y;
  int sx = x >> 31;
  int sy = y >> 31;
  int ss = s >> 31;
  int overflow = (!(sx ^ sy)) & (sx ^ ss);
  return !overflow;
}
