# Week 1 Solutions（位运算练习）

这里放本周的位运算/整数/浮点练习解答。

## 已完成题目（10 题）

1. `p01_bit_xor.c`：仅用 `~` 和 `&` 实现异或
2. `p02_tmin.c`：返回补码最小值
3. `p03_is_tmax.c`：判断是否为补码最大值
4. `p04_all_odd_bits.c`：判断奇数位是否全 1
5. `p05_negate.c`：实现 `-x`
6. `p06_is_ascii_digit.c`：判断是否为 `'0'..'9'`
7. `p07_conditional.c`：无分支实现 `x ? y : z`
8. `p08_is_less_or_equal.c`：实现 `x <= y`
9. `p09_add_ok.c`：判断加法是否溢出
10. `p10_float_scale2.c`：按位实现 `2*f`

## 本地检查

在本目录执行下面命令可做语法检查：

```bash
for f in p*.c; do
  clang -std=c11 -Wall -Wextra -Werror -c "$f" -o /tmp/"${f%.c}".o
done
```
