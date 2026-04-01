# Week 8 — 并发专项收口（竞态 / 死锁 / 性能）

目标：把竞态、同步、死锁、锁粒度这些概念都落到一组小 demo 上，做到“能看懂、能复现、能修”。

## 本周节奏（每天 90 分钟）
Day 1：跑共享计数器的错误版和修复版。  
Day 2：读条件变量版生产者消费者队列。  
Day 3：构造两把锁的循环等待。  
Day 4：统一加锁顺序，消除死锁风险。  
Day 5：总结锁粒度、线程数、吞吐量之间的权衡。

## 本周文件
- `concurrency/race_counter.c`：共享计数器的竞态版和加锁版
- `concurrency/prodcons_queue.c`：最小可用生产者消费者队列
- `concurrency/deadlock_demo.c`：两把锁的死锁模式与修复模式
- `Makefile`
- `notes.md`

## 建议命令

在 `weeks/week8` 目录执行：

```bash
make all
./concurrency/race_counter unsafe 4 200000
./concurrency/race_counter safe 4 200000
./concurrency/prodcons_queue 2 2 20
./concurrency/deadlock_demo deadlock
./concurrency/deadlock_demo ordered
```

## 练习要求
1. Day 1：解释为什么 `unsafe` 版有时错得很多，有时又“看起来正常”。
2. Day 2：指出队列里哪几处必须在持锁状态下访问共享状态。
3. Day 3：写出四个死锁条件里，这个 demo 命中了哪几个。
4. Day 4：说明“统一加锁顺序”为什么能破坏循环等待。
5. Day 5：比较“粗锁简单但并发差”和“细锁复杂但并发高”的 trade-off。

本周交付物：
- 竞态 demo：共享计数器（错误版 + 修复版）
- 生产者消费者队列（最小可用）
- 死锁 demo：两把锁构造死锁 + 修复（统一加锁顺序）
- `notes.md`

## 关联 CSAPP 章节
- 主读：第 12 章《并发编程》
- 重点小节：线程创建与回收、互斥锁 / 条件变量、竞态条件、死锁与规避
- 配套：第 8 章《异常控制流》（理解进程与信号模型差异）
