#include <stdint.h>

/*
 * 题目: 判断 x 是否为补码最大值 Tmax。
 * 运算符语义:
 *   ~a   : 按位取反（0/1 全部翻转）
 *   !a   : 逻辑非（a==0 时为1，否则为0）
 *   !!a  : 把任意非0值规范成1（0保持0）
 *
 * 判定逻辑:
 *   y = x + 1
 *   若 x=Tmax(0x7fffffff)，则 y=Tmin(0x80000000)，x+y=0xffffffff
 *   此时 ~(x+y)=0，所以 !~(x+y)=1。
 *
 *   但 x=-1 时也有 x+y=-1，导致 !~(x+y) 也为 1（干扰项）。
 *   用 !!y 过滤: 当 x=-1 时 y=0，!!y=0；其余情况 !!y=1。
 *
 *   最终: !~(x+y) & !!y
 *   只有 x=Tmax 时返回 1。
 */
int isTmax(int x) {
  int y = x + 1;
  return !~(x + y) & !!y;
}
