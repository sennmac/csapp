# Week 7 — Proxy / 网络 / I/O / 并发

目标：用 socket 写一个“能用”的简化 HTTP 代理，理解 I/O、并发与缓存。

本周交付物：
- `proxy/`：可运行的简化代理
- （可选）简单缓存（LRU 或固定大小）
- `notes.md`

建议里程碑：
1) echo server
2) 解析 HTTP 请求行 + 最小 headers
3) 转发：client → proxy → server → client
4) 并发：线程/线程池（二选一）
5) 缓存（可选）
