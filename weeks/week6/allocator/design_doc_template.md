# Week 6 Design Doc Template

## Data Structure
- 每个 block 的 header 字段是什么
- 空闲块如何串起来
- 为什么保持地址顺序

## Allocation Policy
- 对齐规则
- first-fit / next-fit / best-fit 里你选了哪个
- split 的条件

## Free And Coalescing
- free 时做了哪些状态更新
- coalesce 的触发时机
- 哪些情况还没处理

## Limitations
- 最大 arena
- 时间复杂度热点
- 和官方 `malloclab` 相比还缺什么
