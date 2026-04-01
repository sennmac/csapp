# Week 6 — Malloc / 堆 / 虚拟内存（教学版 malloc）

目标：把虚拟内存与堆管理串起来，理解对齐、碎片、空闲块重用、相邻块合并这些概念在代码里到底长什么样。

## 本周节奏（每天 90 分钟）
Day 1：先用 `vmmap` 或概念图把“栈 / 堆 / 映射区”串起来，再读本周 allocator 的数据结构。  
Day 2：跑通 bump allocator，确认“只向前分配、不回收”的行为。  
Day 3：实现 free list 重用，先做 first-fit 即可。  
Day 4：实现 block split 与相邻空闲块 coalescing。  
Day 5：完成压力回归，并写半页设计文档。

## 本周文件
- `allocator/allocator.c`：教学版 allocator，Day 3/4 留了 TODO
- `allocator/allocator.h`：对外接口
- `allocator/allocator_demo.c`：trace 驱动器
- `allocator/traces/`：按阶段设计的小 trace
- `allocator/design_doc_template.md`：半页设计文档模板
- `Makefile`

## 建议命令

在 `weeks/week6` 目录执行：

```bash
make all
make day2
make day3
make day4
```

## 练习要求
1. Day 2：解释为什么 `free` 之后再次 `alloc` 仍然走到堆尾。
2. Day 3：实现 `find_reusable_block`，让 `day3_freelist.trace` 出现块重用。
3. Day 4：实现 `maybe_split_block` 和 `coalesce_neighbors`，让碎片明显下降。
4. Day 5：把你最终的数据结构、分配策略、失败条件写进设计文档。

## 和官方 malloclab 的衔接
- 参考 handout：[labs/malloclab/malloclab-handout/README](/Users/zhanz/csapp/labs/malloclab/malloclab-handout/README#L14)
- 官方 `mm.c` / `mdriver.c` 适合在你把教学版 allocator 理顺后再接。
- 仓库里的官方 handout 只带了 `short1-bal.rep` 和 `short2-bal.rep`，默认 trace 目录配置仍指向课程服务器，所以这里先给本地 trace 流程。

本周交付物：
- 一个可用的教学版 allocator（bump -> freelist -> split/coalesce）
- 1 份半页设计文档
- `notes.md`

## 关联 CSAPP 章节
- 主读：第 9 章《虚拟内存》
- 重点小节：地址空间、页与映射、动态内存分配、碎片与合并策略
- 配套：第 10 章《系统级 I/O》（`mmap`/文件映射背景，按需选读）
