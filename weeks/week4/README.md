# Week 4 — 安全：栈溢出 / 控制流劫持（教学版）

目标：理解溢出成因、栈帧布局、控制流被覆盖的路径，以及 NX / ASLR / canary 这类防护在现象层面的作用。

注意：只在本机教学场景下观察现象，不做任何未授权攻击。

## 本周节奏（每天 90 分钟）
Day 1：编译并运行 `overflow_demo`，确认局部变量、帧指针、返回地址槽位的大致位置。  
Day 2：用长输入触发崩溃，在 `lldb` 里看回溯、栈内存和损坏后的控制流。  
Day 3：对比 `overflow_demo` 和 `overflow_demo_protected`，记录 stack protector 打开前后的差异。  
Day 4：完成 `ret2func_demo`，先覆盖函数指针到 `touch1`，再挑战把参数一起覆盖到 `touch2`。  
Day 5：整理复盘，写清楚“为什么会崩”“为什么加保护后更难”“哪些信息来自汇编/栈布局而不是猜测”。

## 本周文件
- `overflow_demo.c`：最小栈溢出观察程序
- `ret2func_demo.c`：教学版 ret-to-function 挑战
- `Makefile`：同时编译无保护版和保护版
- `notes.md`：按天复盘模板

## 建议命令

在 `weeks/week4` 目录执行：

```bash
make all
make crash
make protected
make ret2func
make disasm
```

建议配合：

```bash
lldb ./overflow_demo
lldb ./ret2func_demo
nm ./ret2func_demo | rg 'touch|denied'
otool -tvV ./ret2func_demo | less
```

## 练习要求
1. Day 1：写出 `buf`、相邻局部变量、保存的控制信息之间的大致偏移关系。
2. Day 2：分别用 24 / 40 / 56 字节输入，记录程序行为差异。
3. Day 3：比较 `overflow_demo` 与 `overflow_demo_protected` 的报错、退出方式和调试体验。
4. Day 4：完成两关。
   第一关：只改控制流，调用 `touch1`。
   第二关：同时改控制流和参数，调用 `touch2(cookie)`。
5. Day 5：总结 NX / ASLR / canary 各自改变了哪个环节。

## 说明
- Apple Silicon 上真实“覆盖保存返回地址再 ret”可能会受到平台保护影响，Day 4 先用函数指针覆盖练控制流劫持，原理和 ret2func 是同一类思路。
- `overflow_demo` 仍然保留了真实栈局部变量覆盖与崩溃观察，用来练 Day 1-3 的调试和栈分析。

本周交付物：
- 1 个可控的溢出 demo 观察记录
- 1 个完成到 `touch1` / `touch2` 的 payload 说明
- `notes.md`

## 关联 CSAPP 章节
- 主读：第 3 章《程序的机器级表示》
- 重点小节：过程调用约定、栈帧布局、缓冲区溢出相关内容
- 配套：第 7 章《链接》（理解代码布局、重定位与利用面时可参考）
