#include <stdint.h>

/*
 * 题目: 给定 float 的位模式 uf，返回 2*f 的位模式。
 * 字段:
 *   sign: 最高位
 *   exp : 指数字段(8位)
 *   frac: 小数字段(23位)
 *
 * 分类处理:
 * 1) exp==0xFF (NaN/Inf)
 *    按 IEEE754 规则，乘2后仍保持原值编码，直接返回 uf。
 *
 * 2) exp==0 (零或非规格化)
 *    这类值没有隐含前导1，乘2等价于 frac 左移1位。
 *    若左移后顶到 bit23，说明进入规格化区:
 *      exp 置为 1 对应字段 0x00800000，且去掉隐藏位。
 *
 * 3) 其余为规格化数
 *    乘2等价于指数 +1，即 exp += 1<<23。
 *    若加完变成 0xFF，则溢出到 Inf，frac 清零。
 *
 * 返回 sign|exp|frac 即新的位模式。
 */
unsigned floatScale2(unsigned uf) {
  unsigned sign = uf & 0x80000000u;
  unsigned exp = uf & 0x7F800000u;
  unsigned frac = uf & 0x007FFFFFu;

  if (exp == 0x7F800000u) {
    return uf;
  }

  if (exp == 0) {
    frac <<= 1;
    if (frac & 0x00800000u) {
      exp = 0x00800000u;
      frac &= 0x007FFFFFu;
    }
    return sign | exp | frac;
  }

  exp += 0x00800000u;
  if (exp == 0x7F800000u) {
    frac = 0;
  }
  return sign | exp | frac;
}
