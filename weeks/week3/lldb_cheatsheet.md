# LLDB Cheat Sheet（Week 3）

常用：
- 运行：`run` / `r`
- 断点：`b main` / `b file.c:42` / `br list` / `br del <id>`
- 单步：`next` / `n`（源码级）  `step` / `s`
- 汇编：`disassemble -n func`  `disassemble -f`
- 寄存器：`register read`（可加寄存器名）
- 栈回溯：`bt`
- 读内存：`memory read -f x -s4 -c16 $rsp`
- 查看变量：`frame variable`
- 表达式：`expr <c-expr>`

建议：遇到优化导致变量“消失”，用 `disassemble` + `register read` + `memory read` 追踪。
