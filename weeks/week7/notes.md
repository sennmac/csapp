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
  缓存 value：
  淘汰策略：
  命中后如何返回：

- Day 5:
  并发模型：
  需要共享的状态：
  锁粒度：
  可能出现的竞态：
