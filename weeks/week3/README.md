# Week 3 — 反汇编 / 调试（macOS 等价 Bomb）

目标：熟练使用 lldb 在 **汇编层**追踪控制流、栈帧、参数传递；做到“不给源码也能解题”。

本周交付物：
- `lldb_cheatsheet.md`（15 条常用命令 + 用法）
- 1 个自制 mini-bomb（>=3 关），并写出解题记录
- `notes.md`

## 本周文件
- `mini_bomb.c`：4 关 mini-bomb，覆盖字符串、循环、`switch`、递归
- `mini_bomb_solve.md`：解题记录模板
- `Makefile`：`make` / `make run` / `make run-solution` / `make disasm`

## 关联 CSAPP 章节
- 主读：第 3 章《程序的机器级表示》
- 重点小节：数据格式、过程调用与栈帧、控制流、数组与指针在汇编中的映射
- 配套：第 7 章《链接》（理解符号、反汇编与可执行文件结构时有帮助）
