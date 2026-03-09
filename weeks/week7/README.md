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

## 关联 CSAPP 章节
- 主读：第 11 章《网络编程》
- 重点小节：socket、客户端/服务器模型、HTTP 基础请求处理
- 配套：第 10 章《系统级 I/O》（RIO 与健壮 I/O 封装）
- 配套：第 12 章《并发编程》（线程、同步、并发 server 设计）
