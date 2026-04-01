# Week 7 — Proxy / 网络 / I/O / 并发

目标：写一个“能用”的简化 HTTP 代理，先跑通单线程转发，再把结构留好给并发与缓存扩展。

## 本周节奏（每天 90 分钟）
Day 1：读 `proxy.c` 的监听 socket、`accept`、客户端处理入口。  
Day 2：确认 HTTP 请求行和最小 header 的解析逻辑。  
Day 3：跑通 `client -> proxy -> server -> client`。  
Day 4：设计一个固定大小缓存或 LRU 缓存结构。  
Day 5：把单线程版本改成线程版或线程池版。

## 本周文件
- `proxy/proxy.c`：单线程基线代理，Day 4/5 留了扩展点
- `proxy/requests/`：本地手工测试请求
- `Makefile`
- `notes.md`

## 建议命令

在 `weeks/week7` 目录执行：

```bash
make all
make tiny
./proxy/proxy 18080
```

另开一个终端启动 tiny：

```bash
cd ../../labs/proxylab/proxylab-handout/tiny
./tiny 8000
```

然后用 `nc` 测：

```bash
nc 127.0.0.1 18080 < proxy/requests/basic_get.http
```

## 练习要求
1. Day 1：解释监听 socket、连接 socket、客户端 fd、服务端 fd 的角色分工。
2. Day 2：自己加一个 header 透传测试，确认代理只重写 `Host` / `Connection` / `Proxy-Connection` / `User-Agent`。
3. Day 3：记录一次完整转发链路里每个阶段的输入输出。
4. Day 4：设计缓存条目结构，回答“key 是什么、value 是什么、何时淘汰”。
5. Day 5：把 `handle_client` 改造成线程例程，再补锁和缓存一致性。

## 和官方 proxylab 的衔接
- 参考 handout：[labs/proxylab/proxylab-handout/README](/Users/zhanz/csapp/labs/proxylab/proxylab-handout/README#L10)
- `tiny/` 和 `driver.sh` 都在 handout 里，本周目录只放 week-local 的基线实现和测试输入。
- 建议 Day 5 之后再对照官方 `proxy.c` 要求补评分项。

本周交付物：
- `proxy/`：可运行的简化代理
- （可选）简单缓存
- `notes.md`

## 关联 CSAPP 章节
- 主读：第 11 章《网络编程》
- 重点小节：socket、客户端/服务器模型、HTTP 基础请求处理
- 配套：第 10 章《系统级 I/O》（RIO 与健壮 I/O 封装）
- 配套：第 12 章《并发编程》（线程、同步、并发 server 设计）
