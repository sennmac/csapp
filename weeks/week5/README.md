# Week 5 — Shell Lab（迷你 shell）

目标：掌握进程创建、`exec`、等待、重定向、信号处理，并把练习逐步靠近官方 `shlab` 的思路。

## 本周节奏（每天 90 分钟）
Day 1：跑通 `read -> parse -> fork -> exec`。  
Day 2：补齐 PATH 查找、内建命令、基本错误处理。  
Day 3：实现 `>` 重定向，确认子进程文件描述符行为。  
Day 4：加进程组和信号处理，让 `Ctrl-C` / `Ctrl-Z` 不误伤 shell 本身。  
Day 5：做最小可用的 `jobs` / `bg` / `fg`，然后拿 `shlab` 的 trace 做回归。

## 本周文件
- `minish/minish.c`：Day 1-3 可运行的基线 shell，Day 4-5 留了 TODO
- `minish/traces/`：本周本地 smoke trace
- `Makefile`：编译、跑本地 trace、构建 `shlab` 小工具
- `notes.md`

## 建议命令

在 `weeks/week5` 目录执行：

```bash
make all
make day1
make day2
make day3
make shlab-tools
./minish/minish
```

## 练习要求
1. Day 1：理解 `minish.c` 里读命令、切词、`fork`、`execvp` 的最小闭环。
2. Day 2：自己补 2 个内建命令，建议从 `jobs` 的占位版本和 `exit` / `cd` 的边界处理开始。
3. Day 3：验证 `>` 只影响子进程，不污染 shell 的标准输出。
4. Day 4：把 shell 自己放在单独进程组里，为前台 job 建立新的进程组，再转发 `SIGINT` / `SIGTSTP`。
5. Day 5：增加作业表，至少支持：
   `jobs`
   `bg %jid`
   `fg %jid`

## 和官方 shlab 的衔接
- 参考 handout：[labs/shlab/shlab-handout/README](/Users/zhanz/csapp/labs/shlab/shlab-handout/README#L7)
- 推荐优先看的 trace：
  `trace04.txt`：后台任务
  `trace07.txt`：只转发 `SIGINT` 给前台任务
  `trace13.txt`：把停止的进程组拉回前台
- 小工具源码在 `labs/shlab/shlab-handout/`，可用 `make shlab-tools` 先编出来

本周交付物：
- `minish/`：你的迷你 shell 工程（可运行）
- 至少 3 条你自己写的回归输入
- `notes.md`

## 关联 CSAPP 章节
- 主读：第 8 章《异常控制流》
- 重点小节：进程、`fork/exec/wait`、信号与信号处理、作业控制思路
- 配套：第 10 章《系统级 I/O》（文件描述符与重定向实现）
