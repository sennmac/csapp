# Week 2 — Cache / 局部性 / 性能

目标：建立“访问模式 → cache 行为 → 性能”的直觉，并做一次可量化优化。

本周节奏（每天 90 分钟）：20 分钟复习理论，55 分钟编码与测试，15 分钟记录结论。重点不是“背概念”，而是让每次性能变化都能解释清楚。

Day 1：完成矩阵遍历基线实验（row-major 与 col-major），固定矩阵规模、重复次数与编译参数，输出平均耗时。要求写出“为什么会慢/快”的缓存命中解释。  
Day 2：实现简化 cache 模拟器（建议 direct-mapped 起步），支持读取 trace，统计 hit/miss/eviction。先用小样例手算验证，再跑批量输入。  
Day 3：写 baseline transpose，测量不同矩阵维度（如 256/512/1024）下的时间，记录趋势。  
Day 4：加入 blocking（例如 8x8 或 16x16），比较优化前后耗时和 miss 估计，分析最优块大小与平台相关性。  
Day 5：整理复盘，形成“访问模式优化模板”：循环顺序、步长、块大小、数据布局、测量方法。

本周交付物（验收）：
- `bench.c` 或等价程序可一键运行并输出对比数据
- 至少一组 blocking 优化达到可见收益（建议 >=20%）
- `notes.md` 写清 3 个性能坑、3 条有效优化策略、1 条下周风险

建议：优先做 macOS 原生实验；若你后续拿到官方 Cache Lab，再迁移到 `labs/`。

## 关联 CSAPP 章节
- 主读：第 6 章《存储器层次结构》
- 重点小节：局部性原理、cache 组织与命中/缺失、写策略、不同访问模式的性能差异
- 配套：第 5 章《优化程序性能》（循环优化、内存访问顺序、blocking 思想）

## 本周文件与命令
- Day 1：`bench.c`（row-major/col-major + 初步 transpose 对比）
- Day 2：`cache_sim.c` + `traces/sample.trace`（简化 cache 模拟器）
- Day 3：`transpose_baseline.c`（baseline transpose）
- Day 4：`transpose_blocking.c`（blocking 优化）

在 `weeks/week2` 目录执行：

```bash
make all
make day1
make day2
make day3
make day4
```
