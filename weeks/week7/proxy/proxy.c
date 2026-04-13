/*
 * proxy.c — 简化 HTTP 转发代理
 *
 * 功能：
 *   Day 1-3：单线程基线代理，支持 GET 请求转发
 *   Day 4：LRU 缓存（缓存上游响应，重复请求直接返回）
 *   Day 5：多线程并发（每连接一线程 + 缓存加锁）
 *
 * 数据流：
 *   客户端 → recv_request_headers → parse_request → 查缓存
 *     → 命中：直接 send_all 返回缓存内容
 *     → 未命中：open_clientfd → build_forward_request → send_all 到上游
 *              → recv 上游响应 → send_all 回客户端 → 存入缓存
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* ============================================================
 * 常量定义
 * ============================================================ */
enum {
  MAX_REQUEST = 64 * 1024,       /* 客户端请求的最大字节数 */
  MAX_RESPONSE_CHUNK = 16 * 1024,/* 每次从上游读取的缓冲区大小 */
  MAX_HOST = 256,                /* 主机名最大长度 */
  MAX_PORT = 16,                 /* 端口字符串最大长度 */
  MAX_PATH = 2048,               /* URL 路径最大长度 */
  MAX_LINE = 8192,               /* 单行最大长度（错误响应用） */
  MAX_HEADERS = 16 * 1024,       /* 透传 header 的最大总长度 */
  MAX_CACHE_SIZE = 1048576,      /* 缓存总容量上限：1MB */
  MAX_OBJECT_SIZE = 102400,      /* 单个缓存对象上限：100KB */
  MAX_KEY_LEN = MAX_HOST + MAX_PORT + MAX_PATH + 4  /* 缓存 key 最大长度 */
};

/* 统一的 User-Agent，转发时替换客户端原始的 User-Agent */
static const char *USER_AGENT =
    "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3";

/* ============================================================
 * 请求解析结构体
 * ============================================================ */

/* 解析后的 HTTP 请求，包含转发所需的全部信息 */
struct Request {
  char method[16];                /* 请求方法（只支持 GET） */
  char version[16];               /* HTTP 版本（原始值，转发时改为 1.0） */
  char host[MAX_HOST];            /* 上游服务器主机名 */
  char port[MAX_PORT];            /* 上游服务器端口（默认 "80"） */
  char path[MAX_PATH];            /* 请求路径（如 /home.html） */
  char other_headers[MAX_HEADERS];/* 需要透传的其他 header */
};

/* ============================================================
 * LRU 缓存
 *
 * 数据结构：双向链表
 *   - 头部 = 最近使用的条目
 *   - 尾部 = 最久未使用的条目（淘汰时优先删）
 *
 * Key:   "host:port/path"（如 "127.0.0.1:8000/home.html"）
 * Value: 上游返回的完整 HTTP 响应（header + body 的原始字节）
 * ============================================================ */

/* 缓存条目：一个 URL 对应一份响应 */
struct CacheEntry {
  char key[MAX_KEY_LEN];         /* 缓存 key = host:port/path */
  char *value;                   /* 响应数据（堆分配） */
  size_t value_len;              /* 响应数据长度 */
  struct CacheEntry *prev;       /* 双向链表：前一个（更新的） */
  struct CacheEntry *next;       /* 双向链表：后一个（更旧的） */
};

/* 缓存整体结构 */
struct Cache {
  struct CacheEntry *head;       /* 链表头部 = 最近使用 */
  struct CacheEntry *tail;       /* 链表尾部 = 最久未使用 */
  size_t total_size;             /* 当前缓存的总字节数 */
  pthread_mutex_t lock;          /* 互斥锁：多线程下保护缓存读写 */
};

/* 全局缓存实例 */
static struct Cache cache = {
  .head = NULL,
  .tail = NULL,
  .total_size = 0,
  .lock = PTHREAD_MUTEX_INITIALIZER
};

/* ------------------------------------------------------------ */
/* 缓存内部操作（调用前必须已持有 cache.lock）                      */
/* ------------------------------------------------------------ */

/*
 * 把一个条目从链表中摘下来（不释放内存）
 * 用于：移到头部前先摘下来、删除条目前先摘下来
 */
static void cache_detach(struct CacheEntry *entry) {
  if (entry->prev != NULL) {
    entry->prev->next = entry->next;
  } else {
    cache.head = entry->next;    /* 摘的是头节点 */
  }

  if (entry->next != NULL) {
    entry->next->prev = entry->prev;
  } else {
    cache.tail = entry->prev;    /* 摘的是尾节点 */
  }

  entry->prev = NULL;
  entry->next = NULL;
}

/*
 * 把一个条目插入链表头部（标记为"最近使用"）
 */
static void cache_push_front(struct CacheEntry *entry) {
  entry->prev = NULL;
  entry->next = cache.head;

  if (cache.head != NULL) {
    cache.head->prev = entry;
  }
  cache.head = entry;

  if (cache.tail == NULL) {
    cache.tail = entry;          /* 链表为空时，头尾都是它 */
  }
}

/*
 * 释放一个条目的内存
 */
static void cache_free_entry(struct CacheEntry *entry) {
  free(entry->value);
  free(entry);
}

/* ------------------------------------------------------------ */
/* 缓存公开接口（内部加锁，调用者不需要管锁）                      */
/* ------------------------------------------------------------ */

/*
 * cache_find — 查找缓存
 *
 * 输入：key（如 "127.0.0.1:8000/home.html"）
 * 输出：命中时把响应数据复制到 dest，长度写入 *dest_len，返回 0
 *       未命中返回 -1
 *
 * 命中后会把条目移到链表头部（LRU 更新）
 */
static int cache_find(const char *key, char *dest, size_t dest_cap,
                      size_t *dest_len) {
  struct CacheEntry *p;

  pthread_mutex_lock(&cache.lock);

  for (p = cache.head; p != NULL; p = p->next) {
    if (strcmp(p->key, key) == 0) {
      /* 命中：检查目标 buffer 是否够大 */
      if (p->value_len > dest_cap) {
        pthread_mutex_unlock(&cache.lock);
        return -1;
      }
      /* 复制响应数据到调用者的 buffer */
      memcpy(dest, p->value, p->value_len);
      *dest_len = p->value_len;
      /* LRU 更新：移到头部 */
      cache_detach(p);
      cache_push_front(p);
      pthread_mutex_unlock(&cache.lock);
      printf("proxy: 缓存命中 %s (%zu bytes)\n", key, p->value_len);
      return 0;
    }
  }

  pthread_mutex_unlock(&cache.lock);
  return -1;  /* 未命中 */
}

/*
 * cache_insert — 插入缓存
 *
 * 输入：key、响应数据 value、长度 value_len
 * 行为：如果 value_len > MAX_OBJECT_SIZE，不缓存
 *       如果总容量超限，从尾部驱逐最久未使用的条目
 *       如果 key 已存在，更新它的内容
 */
static void cache_insert(const char *key, const char *value, size_t value_len) {
  struct CacheEntry *entry;
  struct CacheEntry *p;
  char *value_copy;

  /* 单个对象太大，不缓存 */
  if (value_len > (size_t)MAX_OBJECT_SIZE) {
    return;
  }

  value_copy = malloc(value_len);
  if (value_copy == NULL) {
    return;
  }
  memcpy(value_copy, value, value_len);

  pthread_mutex_lock(&cache.lock);

  /* 如果 key 已存在，先删掉旧条目 */
  for (p = cache.head; p != NULL; p = p->next) {
    if (strcmp(p->key, key) == 0) {
      cache_detach(p);
      cache.total_size -= p->value_len;
      cache_free_entry(p);
      break;
    }
  }

  /* 驱逐：从尾部删除，直到有足够空间 */
  while (cache.total_size + value_len > (size_t)MAX_CACHE_SIZE &&
         cache.tail != NULL) {
    struct CacheEntry *victim = cache.tail;
    printf("proxy: 缓存驱逐 %s (%zu bytes)\n", victim->key, victim->value_len);
    cache_detach(victim);
    cache.total_size -= victim->value_len;
    cache_free_entry(victim);
  }

  /* 创建新条目 */
  entry = malloc(sizeof(*entry));
  if (entry == NULL) {
    free(value_copy);
    pthread_mutex_unlock(&cache.lock);
    return;
  }
  snprintf(entry->key, sizeof(entry->key), "%s", key);
  entry->value = value_copy;
  entry->value_len = value_len;
  entry->prev = NULL;
  entry->next = NULL;

  /* 插入头部 */
  cache_push_front(entry);
  cache.total_size += value_len;

  pthread_mutex_unlock(&cache.lock);
  printf("proxy: 缓存写入 %s (%zu bytes, 总计 %zu/%d)\n",
         key, value_len, cache.total_size, MAX_CACHE_SIZE);
}

/* ============================================================
 * 字符串工具函数
 * ============================================================ */

/*
 * copy_string — 安全字符串复制
 *
 * 用 snprintf 复制字符串，防止溢出。
 * 返回 0 = 成功，-1 = 目标 buffer 不够大
 */
static int copy_string(char *dst, size_t dst_cap, const char *src) {
  int written = snprintf(dst, dst_cap, "%s", src);
  return written < 0 || (size_t)written >= dst_cap ? -1 : 0;
}

/*
 * trim_spaces — 去除字符串首尾空白
 *
 * 原地修改（尾部写 '\0'），返回跳过前导空白后的指针。
 * 用于解析 "Host:  example.com  " → "example.com"
 */
static char *trim_spaces(char *s) {
  size_t len;

  /* 跳过前导空白 */
  while (*s != '\0' && isspace((unsigned char)*s)) {
    ++s;
  }

  /* 去掉尾部空白 */
  len = strlen(s);
  while (len > 0 && isspace((unsigned char)s[len - 1])) {
    s[len - 1] = '\0';
    --len;
  }

  return s;
}

/* ============================================================
 * HTTP 请求解析
 * ============================================================ */

/*
 * split_host_port — 从 "host:port" 字符串中拆分主机名和端口
 *
 * 输入：input = "127.0.0.1:8000" 或 "example.com"
 * 输出：host = "127.0.0.1", port = "8000"
 *       如果没有冒号，port 默认 "80"
 */
static int split_host_port(const char *input, char *host, size_t host_cap,
                           char *port, size_t port_cap) {
  /* 找最后一个冒号（避免 IPv6 地址中的冒号干扰） */
  const char *colon = strrchr(input, ':');

  if (colon != NULL && strchr(colon + 1, ':') == NULL && strchr(input, ']') == NULL) {
    /* 有冒号且不是 IPv6 格式：拆分 host 和 port */
    size_t host_len = (size_t)(colon - input);

    if (host_len == 0 || host_len >= host_cap) {
      return -1;
    }

    memcpy(host, input, host_len);
    host[host_len] = '\0';
    return copy_string(port, port_cap, colon + 1);
  }

  /* 没有端口号，整个 input 就是 host，端口默认 80 */
  if (copy_string(host, host_cap, input) != 0) {
    return -1;
  }
  return copy_string(port, port_cap, "80");
}

/*
 * parse_absolute_uri — 解析绝对 URI
 *
 * 输入：uri = "http://127.0.0.1:8000/home.html"
 * 输出：req->host = "127.0.0.1"
 *       req->port = "8000"
 *       req->path = "/home.html"
 *
 * 代理收到的请求通常是绝对 URI 格式，需要拆出 host/port/path
 */
static int parse_absolute_uri(const char *uri, struct Request *req) {
  const char *prefix = "http://";
  const char *authority;
  const char *slash;
  char host_port[MAX_HOST + MAX_PORT];
  size_t authority_len;

  /* 必须以 http:// 开头 */
  if (strncmp(uri, prefix, strlen(prefix)) != 0) {
    return -1;
  }

  /* 跳过 "http://"，得到 "127.0.0.1:8000/home.html" */
  authority = uri + strlen(prefix);
  /* 找第一个 '/'，分隔 authority 和 path */
  slash = strchr(authority, '/');
  authority_len = slash == NULL ? strlen(authority) : (size_t)(slash - authority);
  if (authority_len == 0 || authority_len >= sizeof(host_port)) {
    return -1;
  }

  /* 提取 authority 部分（"127.0.0.1:8000"） */
  memcpy(host_port, authority, authority_len);
  host_port[authority_len] = '\0';

  /* 从 authority 中拆分 host 和 port */
  if (split_host_port(host_port, req->host, sizeof(req->host), req->port,
                      sizeof(req->port)) != 0) {
    return -1;
  }

  /* 提取 path 部分；如果 URI 没有 path（如 "http://host"），默认 "/" */
  if (slash == NULL) {
    return copy_string(req->path, sizeof(req->path), "/");
  }

  return copy_string(req->path, sizeof(req->path), slash);
}

/*
 * append_header — 把一行 header 追加到 other_headers 缓冲区
 *
 * 每次追加一行，自动加 \r\n 结尾。
 * 返回 -1 表示缓冲区满了。
 */
static int append_header(char *dst, size_t dst_cap, const char *line) {
  size_t used = strlen(dst);
  int written;

  written = snprintf(dst + used, dst_cap - used, "%s\r\n", line);
  return written < 0 || (size_t)written >= dst_cap - used ? -1 : 0;
}

/*
 * parse_request — 解析完整的 HTTP 请求
 *
 * 输入：raw = 客户端发来的原始请求文本
 * 输出：req 结构体被填充
 *
 * 返回值：
 *    0 = 解析成功
 *   -1 = 格式错误
 *   -2 = 不支持的方法（非 GET）
 *
 * 处理逻辑：
 *   1. 解析请求行：方法、URI、版本
 *   2. 如果 URI 是绝对 URI（http://...），从中提取 host/port/path
 *   3. 遍历 header：
 *      - Host: 提取主机（URI 优先级高于 Host header）
 *      - Connection / Proxy-Connection / User-Agent: 丢弃（转发时统一重写）
 *      - 其他 header: 原样透传
 */
static int parse_request(char *raw, struct Request *req) {
  char *line;
  char *save = NULL;
  char uri[MAX_PATH];
  bool host_from_uri = false;

  memset(req, 0, sizeof(*req));
  /* 端口默认 80 */
  if (copy_string(req->port, sizeof(req->port), "80") != 0) {
    return -1;
  }

  /* 第一行：请求行 "GET http://... HTTP/1.0" */
  line = strtok_r(raw, "\r\n", &save);
  if (line == NULL) {
    return -1;
  }

  /* 拆分请求行的三个部分 */
  if (sscanf(line, "%15s %2047s %15s", req->method, uri, req->version) != 3) {
    return -1;
  }

  /* 只支持 GET */
  if (strcmp(req->method, "GET") != 0) {
    return -2;
  }

  /* 判断 URI 类型：绝对 URI 还是普通路径 */
  if (strncmp(uri, "http://", 7) == 0) {
    if (parse_absolute_uri(uri, req) != 0) {
      return -1;
    }
    host_from_uri = true;  /* URI 里已经有 host 了，忽略 Host header */
  } else if (copy_string(req->path, sizeof(req->path), uri) != 0) {
    return -1;
  }

  /* 遍历剩余的 header 行 */
  while ((line = strtok_r(NULL, "\r\n", &save)) != NULL) {
    char *value;

    if (*line == '\0') {
      continue;
    }

    if (strncasecmp(line, "Host:", 5) == 0) {
      /* Host header：如果 URI 里没有 host，从这里取 */
      value = trim_spaces(line + 5);
      if (!host_from_uri &&
          split_host_port(value, req->host, sizeof(req->host), req->port,
                          sizeof(req->port)) != 0) {
        return -1;
      }
    } else if (strncasecmp(line, "Connection:", 11) == 0 ||
               strncasecmp(line, "Proxy-Connection:", 17) == 0 ||
               strncasecmp(line, "User-Agent:", 11) == 0) {
      /* 这三个 header 丢弃，转发时统一重写 */
      continue;
    } else if (append_header(req->other_headers, sizeof(req->other_headers), line) != 0) {
      /* 其他 header 原样透传 */
      return -1;
    }
  }

  /* 必须有 host，否则不知道连谁 */
  if (req->host[0] == '\0') {
    return -1;
  }

  return 0;
}

/* ============================================================
 * 网络 I/O 工具函数
 * ============================================================ */

/*
 * send_all — 循环发送，保证所有数据都发出去
 *
 * send() 可能只发出一部分（短计数），所以必须循环。
 * 返回 0 = 全部发完，-1 = 出错
 */
static int send_all(int fd, const void *buf, size_t len) {
  const char *ptr = (const char *)buf;

  while (len > 0) {
    ssize_t nwritten = send(fd, ptr, len, 0);
    if (nwritten <= 0) {
      return -1;   /* 出错或对端关闭 */
    }
    ptr += nwritten;           /* 指针前移，跳过已发部分 */
    len -= (size_t)nwritten;   /* 剩余量减少 */
  }

  return 0;
}

/*
 * recv_request_headers — 从客户端读取完整的 HTTP 请求头
 *
 * 持续 recv，直到看到 "\r\n\r\n"（header 结束标志）。
 * 返回 0 = 读到完整 header，-1 = 出错或 buffer 满了
 */
static int recv_request_headers(int fd, char *buf, size_t cap) {
  size_t used = 0;

  while (used + 1 < cap) {
    ssize_t nread = recv(fd, buf + used, cap - used - 1, 0);
    if (nread <= 0) {
      return -1;   /* 对端关闭或出错 */
    }
    used += (size_t)nread;
    buf[used] = '\0';          /* 保持字符串以 '\0' 结尾 */
    if (strstr(buf, "\r\n\r\n") != NULL) {
      return 0;    /* 找到 header 结束标志 */
    }
  }

  return -1;  /* buffer 满了还没看到 \r\n\r\n */
}

/* ============================================================
 * Socket 创建函数
 * ============================================================ */

/*
 * open_listenfd — 创建监听 socket
 *
 * 流程：getaddrinfo → socket → setsockopt → bind → listen
 * 遍历地址列表（可能有 IPv4/IPv6 多个），直到成功。
 *
 * SO_REUSEADDR：允许在 TIME_WAIT 期间重新绑定端口（方便调试重启）
 * backlog=1024：accept 队列最多排 1024 个已完成握手的连接
 */
static int open_listenfd(const char *port) {
  struct addrinfo hints;
  struct addrinfo *listp = NULL;
  struct addrinfo *p;
  int listenfd = -1;
  int optval = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;   /* TCP */
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
  /* AI_PASSIVE：用于 bind（服务器端） */
  /* AI_ADDRCONFIG：只返回本机支持的地址族 */
  /* AI_NUMERICSERV：端口参数是数字字符串 */

  if (getaddrinfo(NULL, port, &hints, &listp) != 0) {
    return -1;
  }

  /* 遍历地址列表，尝试绑定 */
  for (p = listp; p != NULL; p = p->ai_next) {
    listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listenfd < 0) {
      continue;   /* 这个地址创建 socket 失败，试下一个 */
    }

    /* 允许端口复用 */
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0 &&
        listen(listenfd, 1024) == 0) {
      break;      /* 绑定+监听成功 */
    }

    close(listenfd);
    listenfd = -1;
  }

  freeaddrinfo(listp);
  return listenfd;
}

/*
 * open_clientfd — 创建连接 socket（代理作为客户端连接上游）
 *
 * 流程：getaddrinfo → socket → connect
 * 遍历地址列表，直到连接成功。
 *
 * 这是代理"变身客户端"的地方：主动 connect 上游服务器。
 */
static int open_clientfd(const char *host, const char *port) {
  struct addrinfo hints;
  struct addrinfo *listp = NULL;
  struct addrinfo *p;
  int clientfd = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;   /* TCP */
  hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

  if (getaddrinfo(host, port, &hints, &listp) != 0) {
    return -1;
  }

  /* 遍历地址列表，尝试连接 */
  for (p = listp; p != NULL; p = p->ai_next) {
    clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (clientfd < 0) {
      continue;
    }

    if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0) {
      break;      /* 连接成功（三次握手完成） */
    }

    close(clientfd);
    clientfd = -1;
  }

  freeaddrinfo(listp);
  return clientfd;
}

/* ============================================================
 * HTTP 响应构造
 * ============================================================ */

/*
 * send_error — 向客户端发送错误响应
 *
 * 构造一个完整的 HTTP/1.0 错误响应（status + header + body），
 * 用 send_all 发给客户端。
 */
static void send_error(int clientfd, const char *status, const char *message) {
  char buf[MAX_LINE];
  int written = snprintf(buf, sizeof(buf),
                         "HTTP/1.0 %s\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s",
                         status, strlen(message), message);
  if (written > 0 && (size_t)written < sizeof(buf)) {
    (void)send_all(clientfd, buf, (size_t)written);
  }
}

/*
 * build_forward_request — 构造发给上游服务器的 HTTP 请求
 *
 * 输入：解析好的 req 结构体
 * 输出：buf 被填入重写后的请求文本
 *
 * 重写规则：
 *   - 请求行：GET <path> HTTP/1.0（版本降为 1.0）
 *   - Host: 使用解析出的 host[:port]
 *   - User-Agent: 统一替换
 *   - Connection: close（避免持久连接的复杂性）
 *   - Proxy-Connection: close
 *   - 其他 header: 原样透传
 *
 * 返回写入的字节数，-1 = 失败
 */
static int build_forward_request(const struct Request *req, char *buf, size_t cap) {
  char host_header[MAX_HOST + MAX_PORT + 2];
  int written;

  /* 构造 Host header：端口是 80 时省略端口号 */
  if (strcmp(req->port, "80") == 0) {
    if (copy_string(host_header, sizeof(host_header), req->host) != 0) {
      return -1;
    }
  } else {
    written = snprintf(host_header, sizeof(host_header), "%s:%s", req->host, req->port);
    if (written < 0 || (size_t)written >= sizeof(host_header)) {
      return -1;
    }
  }

  /* 拼装完整请求 */
  written = snprintf(buf, cap,
                     "GET %s HTTP/1.0\r\n"
                     "Host: %s\r\n"
                     "User-Agent: %s\r\n"
                     "Connection: close\r\n"
                     "Proxy-Connection: close\r\n"
                     "%s"
                     "\r\n",
                     req->path,
                     host_header,
                     USER_AGENT,
                     req->other_headers);
  return written < 0 || (size_t)written >= cap ? -1 : written;
}

/* ============================================================
 * 核心转发逻辑
 * ============================================================ */

/*
 * make_cache_key — 构造缓存 key
 *
 * 格式："host:port/path"，如 "127.0.0.1:8000/home.html"
 */
static void make_cache_key(const struct Request *req, char *key, size_t key_cap) {
  snprintf(key, key_cap, "%s:%s%s", req->host, req->port, req->path);
}

/*
 * handle_client — 处理一个客户端请求的完整流程
 *
 * 五步：
 *   1. recv_request_headers：从客户端读请求
 *   2. parse_request：解析出 host/port/path
 *   [缓存] 查缓存 → 命中则直接返回
 *   3. open_clientfd：连接上游服务器
 *   4. build_forward_request + send_all：构造并发送转发请求
 *   5. recv 循环：读上游响应 → send_all 转发回客户端
 *   [缓存] 转发完后存入缓存
 */
static void handle_client(int clientfd) {
  char raw[MAX_REQUEST];
  char outbound[MAX_REQUEST];
  char response[MAX_RESPONSE_CHUNK];
  struct Request req;
  int serverfd;
  int outbound_len;
  char cache_key[MAX_KEY_LEN];

  /* 用于收集响应数据以存入缓存 */
  char *cached_response = NULL;
  size_t cached_len = 0;
  bool cacheable = true;

  /* ---------- 第 1 步：读客户端请求 ---------- */
  if (recv_request_headers(clientfd, raw, sizeof(raw)) != 0) {
    send_error(clientfd, "400 Bad Request", "failed to read request");
    return;
  }

  /* ---------- 第 2 步：解析请求 ---------- */
  switch (parse_request(raw, &req)) {
    case 0:
      break;   /* 解析成功 */
    case -2:
      send_error(clientfd, "501 Not Implemented", "only GET is supported");
      return;
    default:
      send_error(clientfd, "400 Bad Request", "could not parse request");
      return;
  }

  /* ---------- 缓存查找 ---------- */
  make_cache_key(&req, cache_key, sizeof(cache_key));
  {
    /* 用栈上 buffer 接收缓存数据（最大 MAX_OBJECT_SIZE） */
    char cache_buf[MAX_OBJECT_SIZE];
    size_t cache_hit_len;

    if (cache_find(cache_key, cache_buf, sizeof(cache_buf), &cache_hit_len) == 0) {
      /* 缓存命中：直接发给客户端，跳过上游连接 */
      send_all(clientfd, cache_buf, cache_hit_len);
      return;
    }
  }

  /* ---------- 第 3 步：连接上游服务器 ---------- */
  serverfd = open_clientfd(req.host, req.port);
  if (serverfd < 0) {
    send_error(clientfd, "502 Bad Gateway", "failed to connect upstream");
    return;
  }

  /* ---------- 第 4 步：构造并发送转发请求 ---------- */
  outbound_len = build_forward_request(&req, outbound, sizeof(outbound));
  if (outbound_len < 0 || send_all(serverfd, outbound, (size_t)outbound_len) != 0) {
    close(serverfd);
    send_error(clientfd, "502 Bad Gateway", "failed to forward request");
    return;
  }

  printf("proxy: %s %s -> %s:%s\n", req.method, req.path, req.host, req.port);

  /* ---------- 第 5 步：读上游响应并转发回客户端 ---------- */
  /* 同时把响应累积到 cached_response，用于后续存入缓存 */
  while (1) {
    ssize_t nread = recv(serverfd, response, sizeof(response), 0);
    if (nread < 0) {
      break;     /* 读取出错 */
    }
    if (nread == 0) {
      break;     /* 上游关闭连接（响应发完了） */
    }

    /* 转发给客户端 */
    if (send_all(clientfd, response, (size_t)nread) != 0) {
      break;     /* 客户端断了 */
    }

    /* 累积响应数据以备缓存 */
    if (cacheable) {
      if (cached_len + (size_t)nread > (size_t)MAX_OBJECT_SIZE) {
        /* 响应太大，放弃缓存 */
        cacheable = false;
        free(cached_response);
        cached_response = NULL;
      } else {
        char *tmp = realloc(cached_response, cached_len + (size_t)nread);
        if (tmp == NULL) {
          cacheable = false;
          free(cached_response);
          cached_response = NULL;
        } else {
          cached_response = tmp;
          memcpy(cached_response + cached_len, response, (size_t)nread);
          cached_len += (size_t)nread;
        }
      }
    }
  }

  close(serverfd);

  /* ---------- 缓存写入 ---------- */
  if (cacheable && cached_response != NULL && cached_len > 0) {
    cache_insert(cache_key, cached_response, cached_len);
  }
  free(cached_response);
}

/* ============================================================
 * 多线程支持（Day 5）
 * ============================================================ */

/*
 * thread_routine — 线程入口函数
 *
 * 每个线程处理一个客户端连接：
 *   1. 从 arg 中取出 clientfd（malloc 的副本）
 *   2. 调用 handle_client 完成转发
 *   3. close fd，free 副本
 *
 * 为什么用 malloc 传 fd：
 *   如果直接传 &clientfd，主循环下次 accept 会覆盖 clientfd 的值，
 *   线程可能读到错误的 fd。malloc 一份副本保证每个线程独立。
 */
static void *thread_routine(void *arg) {
  int clientfd = *(int *)arg;
  free(arg);                    /* 释放 malloc 的副本 */
  handle_client(clientfd);      /* 完整处理这个客户端的请求 */
  close(clientfd);              /* 线程负责关闭连接 */
  return NULL;
}

/* ============================================================
 * 主函数
 * ============================================================ */
int main(int argc, char **argv) {
  int listenfd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <listen_port>\n", argv[0]);
    return 1;
  }

  /*
   * 忽略 SIGPIPE：
   * 如果客户端提前断开，往 clientfd 写数据会触发 SIGPIPE，
   * 默认行为是终止进程。忽略后 send 返回 -1，代理可以正常处理。
   */
  signal(SIGPIPE, SIG_IGN);

  listenfd = open_listenfd(argv[1]);
  if (listenfd < 0) {
    perror("open_listenfd");
    return 1;
  }

  printf("proxy listening on port %s\n", argv[1]);

  /*
   * 主循环：永远等待新连接
   *
   * accept 阻塞直到有客户端连入 → 返回新的 clientfd
   * 创建新线程处理这个连接，主循环立刻回到 accept 等下一个
   * 多个客户端可以同时被服务（并发）
   */
  while (1) {
    struct sockaddr_storage clientaddr;  /* 存客户端的 IP + 端口 */
    socklen_t clientlen = sizeof(clientaddr);
    int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    if (clientfd < 0) {
      perror("accept");
      continue;   /* accept 失败不退出，继续等下一个 */
    }

    /* 创建线程处理连接 */
    int *fdp = malloc(sizeof(int));  /* malloc 一份 fd 副本给线程 */
    if (fdp == NULL) {
      close(clientfd);
      continue;
    }
    *fdp = clientfd;

    pthread_t tid;
    pthread_create(&tid, NULL, thread_routine, fdp);
    pthread_detach(tid);  /* 分离线程：不需要 join，线程结束自动回收资源 */
  }

  return 0;
}
