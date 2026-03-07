# Week 5 — Shell Lab（迷你 shell）

目标：掌握进程创建、exec、等待、信号处理，并实现简化版作业控制。

本周交付物：
- `minish/`：你的迷你 shell 工程（可运行）
- `notes.md`

建议里程碑：
1) `readline -> fork -> exec` 跑通
2) 参数解析 + PATH 查找
3) 重定向（先 `>`）
4) 信号（Ctrl-C / Ctrl-Z）不误伤 shell
5) 简化 bg/fg（目标是“能用”，不追求 bash 完整）
