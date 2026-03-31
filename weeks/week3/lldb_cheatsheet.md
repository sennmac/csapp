# LLDB Cheat Sheet（Week 3）

常用：
- 运行：`run` / `r`
- 断点：`b main` / `b file.c:42` / `br list` / `br del <id>`
- 单步：`next` / `n`（源码级）  `step` / `s`
- 指令级单步：`thread step-inst` / `si`  `thread step-inst-over` / `ni`
- 跳出当前函数：`finish`
- 汇编：`disassemble -n func`  `disassemble -f`
- 寄存器：`register read`（可加寄存器名）  `register write <reg> <value>`
- 栈回溯：`bt`
- 读内存：`memory read -f x -s4 -c16 $rsp`
- 查看指令地址附近内存：`memory read -f x -s1 -c32 <addr>`
- 查看变量：`frame variable`
- 表达式：`expr <c-expr>`
- 看当前 frame：`frame info`
- 看模块与符号：`image list`  `image lookup -n func`
- 按地址下断：`breakpoint set --address <addr>`
- 删除全部断点：`breakpoint delete`
- 观察点：`watchpoint set variable <name>`

建议：遇到优化导致变量“消失”，用 `disassemble` + `register read` + `memory read` 追踪。
