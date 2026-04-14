# Week 7 Notes

- Day 1:
  监听 socket 和连接 socket 的区别：
  `listenfd` 只负责 `bind` + `listen`，挂在代理自己的端口上等待新连接，不直接承载某个 HTTP 请求的数据流。
  `clientfd` 是 `accept` 返回的已连接 socket，负责从浏览器/客户端读取请求头，再把上游响应写回去。
  `serverfd` 是代理主动连上游服务器时得到的连接 socket，负责把改写后的请求发给 tiny，再把 tiny 的响应读回来。
  `accept` 返回的 fd 用在什么地方：
  在 `handle_client(clientfd)` 里读取原始请求、发送错误页或最终响应，处理完后关闭。
  今天我最容易混淆的一个点：
  `listenfd` 不是“当前客户端连接”；真正和某个客户端一一对应的是 `accept` 之后的 `clientfd`。同一次代理转发里还会额外出现一个独立的 `serverfd`，分别对应下游和上游两段 TCP 连接。

- Day 2:
  请求行解析规则：
  只支持 `GET`；请求目标既可以是 `http://host:port/path` 这种 absolute URI，也可以是 `/path` 配合 `Host:`。如果 URI/Host 里没写端口，默认用 `80`。
  哪些 header 被重写：
  代理转发时固定生成 `Host`、`User-Agent`、`Connection: close`、`Proxy-Connection: close`，并且统一把请求行改写成 `GET <path> HTTP/1.0`。
  哪些 header 被透传：
  除了 `Host` / `Connection` / `Proxy-Connection` / `User-Agent` 之外，其余 header 会原样追加到 `other_headers`，例如我实际验证过 `X-Debug: week7` 和 `Accept: text/plain` 都能到达 tiny。

- Day 3:
  一次完整转发链路：
  客户端连 `127.0.0.1:18080` 发 `GET http://127.0.0.1:8000/home.html HTTP/1.0`；代理解析出 `host=127.0.0.1`、`port=8000`、`path=/home.html`；代理再连 tiny，把规范化后的请求发给 tiny；tiny 返回 `home.html`；代理把响应分块读出并直接转发回客户端。
  上游连接失败时我的处理：
  读请求头失败返回 `400 Bad Request`；方法不是 `GET` 返回 `501 Not Implemented`；连不上上游或转发失败返回 `502 Bad Gateway`。
  我如何验证代理真的工作了：
  先在 `weeks/week7` 下 `make all`，再构建并启动 handout 里的 `tiny 8000`，启动 `./proxy/proxy 18080`，然后用 `curl -sS -x http://127.0.0.1:18080 http://127.0.0.1:8000/home.html` 拉到了页面内容；额外用带 `X-Debug` 和 `Accept` 的请求确认 tiny 端确实收到了透传 header。

- Day 4:
  缓存 key：
  `host:port/path`（如 `127.0.0.1:8000/home.html`），由 `make_cache_key` 用 `req.host`、`req.port`、`req.path` 拼接而成。
  缓存 value：
  上游返回的完整 HTTP 响应（header + body 的原始字节），在转发循环中用 `realloc` 逐块累积。
  淘汰策略：
  LRU（最近最少使用）。双向链表实现，头部 = 最近访问，尾部 = 最久未访问。容量超过 1MB（`MAX_CACHE_SIZE`）时从尾部驱逐；单个对象超过 100KB（`MAX_OBJECT_SIZE`）不缓存。
  命中后如何返回：
  在 `handle_client` 中解析完请求后、连上游之前调 `cache_find`；命中时把缓存数据复制到栈上 buffer，直接 `send_all` 给客户端，跳过整个上游转发流程。

- Day 5:
  并发模型：
  每连接一线程。`accept` 返回后 `malloc` 一份 fd 副本，`pthread_create` 创建新线程，`pthread_detach` 分离（不需要 join，线程结束自动回收）。主循环立刻回到 `accept` 等下一个连接。
  需要共享的状态：
  全局缓存 `struct Cache cache`（head/tail/total_size 和整条链表）。每个线程的 `handle_client` 都可能查缓存和写缓存。
  锁粒度：
  粗粒度：一把 `pthread_mutex_t` 锁整个缓存。`cache_find` 和 `cache_insert` 内部加锁解锁，内层操作函数（`cache_detach`、`cache_push_front`）不加锁，避免同一线程重复加锁导致死锁。
  可能出现的竞态：
  1. 多个线程同时对同一个 URL 请求上游（缓存未命中），各自写入缓存时后写的覆盖先写的（丢失更新）——用 mutex 保护写入操作解决。2. 一个线程遍历链表时另一个线程修改链表结构——用同一把 mutex 保证同时只有一个线程操作链表。
