#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

enum {
  MAX_REQUEST = 64 * 1024,
  MAX_RESPONSE_CHUNK = 16 * 1024,
  MAX_HOST = 256,
  MAX_PORT = 16,
  MAX_PATH = 2048,
  MAX_LINE = 8192,
  MAX_HEADERS = 16 * 1024
};

static const char *USER_AGENT =
    "Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3";

struct Request {
  char method[16];
  char version[16];
  char host[MAX_HOST];
  char port[MAX_PORT];
  char path[MAX_PATH];
  char other_headers[MAX_HEADERS];
};

static int copy_string(char *dst, size_t dst_cap, const char *src) {
  int written = snprintf(dst, dst_cap, "%s", src);
  return written < 0 || (size_t)written >= dst_cap ? -1 : 0;
}

static char *trim_spaces(char *s) {
  size_t len;

  while (*s != '\0' && isspace((unsigned char)*s)) {
    ++s;
  }

  len = strlen(s);
  while (len > 0 && isspace((unsigned char)s[len - 1])) {
    s[len - 1] = '\0';
    --len;
  }

  return s;
}

static int split_host_port(const char *input, char *host, size_t host_cap,
                           char *port, size_t port_cap) {
  const char *colon = strrchr(input, ':');

  if (colon != NULL && strchr(colon + 1, ':') == NULL && strchr(input, ']') == NULL) {
    size_t host_len = (size_t)(colon - input);

    if (host_len == 0 || host_len >= host_cap) {
      return -1;
    }

    memcpy(host, input, host_len);
    host[host_len] = '\0';
    return copy_string(port, port_cap, colon + 1);
  }

  if (copy_string(host, host_cap, input) != 0) {
    return -1;
  }
  return copy_string(port, port_cap, "80");
}

static int parse_absolute_uri(const char *uri, struct Request *req) {
  const char *prefix = "http://";
  const char *authority;
  const char *slash;
  char host_port[MAX_HOST + MAX_PORT];
  size_t authority_len;

  if (strncmp(uri, prefix, strlen(prefix)) != 0) {
    return -1;
  }

  authority = uri + strlen(prefix);
  slash = strchr(authority, '/');
  authority_len = slash == NULL ? strlen(authority) : (size_t)(slash - authority);
  if (authority_len == 0 || authority_len >= sizeof(host_port)) {
    return -1;
  }

  memcpy(host_port, authority, authority_len);
  host_port[authority_len] = '\0';

  if (split_host_port(host_port, req->host, sizeof(req->host), req->port,
                      sizeof(req->port)) != 0) {
    return -1;
  }

  if (slash == NULL) {
    return copy_string(req->path, sizeof(req->path), "/");
  }

  return copy_string(req->path, sizeof(req->path), slash);
}

static int append_header(char *dst, size_t dst_cap, const char *line) {
  size_t used = strlen(dst);
  int written;

  written = snprintf(dst + used, dst_cap - used, "%s\r\n", line);
  return written < 0 || (size_t)written >= dst_cap - used ? -1 : 0;
}

static int parse_request(char *raw, struct Request *req) {
  char *line;
  char *save = NULL;
  char uri[MAX_PATH];
  bool host_from_uri = false;

  memset(req, 0, sizeof(*req));
  if (copy_string(req->port, sizeof(req->port), "80") != 0) {
    return -1;
  }

  line = strtok_r(raw, "\r\n", &save);
  if (line == NULL) {
    return -1;
  }

  if (sscanf(line, "%15s %2047s %15s", req->method, uri, req->version) != 3) {
    return -1;
  }

  if (strcmp(req->method, "GET") != 0) {
    return -2;
  }

  if (strncmp(uri, "http://", 7) == 0) {
    if (parse_absolute_uri(uri, req) != 0) {
      return -1;
    }
    host_from_uri = true;
  } else if (copy_string(req->path, sizeof(req->path), uri) != 0) {
    return -1;
  }

  while ((line = strtok_r(NULL, "\r\n", &save)) != NULL) {
    char *value;

    if (*line == '\0') {
      continue;
    }

    if (strncasecmp(line, "Host:", 5) == 0) {
      value = trim_spaces(line + 5);
      if (!host_from_uri &&
          split_host_port(value, req->host, sizeof(req->host), req->port,
                          sizeof(req->port)) != 0) {
        return -1;
      }
    } else if (strncasecmp(line, "Connection:", 11) == 0 ||
               strncasecmp(line, "Proxy-Connection:", 17) == 0 ||
               strncasecmp(line, "User-Agent:", 11) == 0) {
      continue;
    } else if (append_header(req->other_headers, sizeof(req->other_headers), line) != 0) {
      return -1;
    }
  }

  if (req->host[0] == '\0') {
    return -1;
  }

  return 0;
}

static int send_all(int fd, const void *buf, size_t len) {
  const char *ptr = (const char *)buf;

  while (len > 0) {
    ssize_t nwritten = send(fd, ptr, len, 0);
    if (nwritten <= 0) {
      return -1;
    }
    ptr += nwritten;
    len -= (size_t)nwritten;
  }

  return 0;
}

static int recv_request_headers(int fd, char *buf, size_t cap) {
  size_t used = 0;

  while (used + 1 < cap) {
    ssize_t nread = recv(fd, buf + used, cap - used - 1, 0);
    if (nread <= 0) {
      return -1;
    }
    used += (size_t)nread;
    buf[used] = '\0';
    if (strstr(buf, "\r\n\r\n") != NULL) {
      return 0;
    }
  }

  return -1;
}

static int open_listenfd(const char *port) {
  struct addrinfo hints;
  struct addrinfo *listp = NULL;
  struct addrinfo *p;
  int listenfd = -1;
  int optval = 1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

  if (getaddrinfo(NULL, port, &hints, &listp) != 0) {
    return -1;
  }

  for (p = listp; p != NULL; p = p->ai_next) {
    listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (listenfd < 0) {
      continue;
    }

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0 &&
        listen(listenfd, 1024) == 0) {
      break;
    }

    close(listenfd);
    listenfd = -1;
  }

  freeaddrinfo(listp);
  return listenfd;
}

static int open_clientfd(const char *host, const char *port) {
  struct addrinfo hints;
  struct addrinfo *listp = NULL;
  struct addrinfo *p;
  int clientfd = -1;

  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

  if (getaddrinfo(host, port, &hints, &listp) != 0) {
    return -1;
  }

  for (p = listp; p != NULL; p = p->ai_next) {
    clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (clientfd < 0) {
      continue;
    }

    if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0) {
      break;
    }

    close(clientfd);
    clientfd = -1;
  }

  freeaddrinfo(listp);
  return clientfd;
}

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

static int build_forward_request(const struct Request *req, char *buf, size_t cap) {
  char host_header[MAX_HOST + MAX_PORT + 2];
  int written;

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

static void handle_client(int clientfd) {
  char raw[MAX_REQUEST];
  char outbound[MAX_REQUEST];
  char response[MAX_RESPONSE_CHUNK];
  struct Request req;
  int serverfd;
  int outbound_len;

  if (recv_request_headers(clientfd, raw, sizeof(raw)) != 0) {
    send_error(clientfd, "400 Bad Request", "failed to read request");
    return;
  }

  switch (parse_request(raw, &req)) {
    case 0:
      break;
    case -2:
      send_error(clientfd, "501 Not Implemented", "only GET is supported");
      return;
    default:
      send_error(clientfd, "400 Bad Request", "could not parse request");
      return;
  }

  serverfd = open_clientfd(req.host, req.port);
  if (serverfd < 0) {
    send_error(clientfd, "502 Bad Gateway", "failed to connect upstream");
    return;
  }

  outbound_len = build_forward_request(&req, outbound, sizeof(outbound));
  if (outbound_len < 0 || send_all(serverfd, outbound, (size_t)outbound_len) != 0) {
    close(serverfd);
    send_error(clientfd, "502 Bad Gateway", "failed to forward request");
    return;
  }

  printf("proxy: %s %s -> %s:%s\n", req.method, req.path, req.host, req.port);

  while (1) {
    ssize_t nread = recv(serverfd, response, sizeof(response), 0);
    if (nread < 0) {
      break;
    }
    if (nread == 0) {
      break;
    }
    if (send_all(clientfd, response, (size_t)nread) != 0) {
      break;
    }
  }

  close(serverfd);
}

int main(int argc, char **argv) {
  int listenfd;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <listen_port>\n", argv[0]);
    return 1;
  }

  signal(SIGPIPE, SIG_IGN);

  listenfd = open_listenfd(argv[1]);
  if (listenfd < 0) {
    perror("open_listenfd");
    return 1;
  }

  printf("proxy listening on port %s\n", argv[1]);
  while (1) {
    struct sockaddr_storage clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    if (clientfd < 0) {
      perror("accept");
      continue;
    }

    /*
     * Day 5 TODO:
     * Change this direct call into a thread routine or hand the fd to a
     * thread pool. If you add caching, this is also the point where you
     * need to reason about shared state and locking granularity.
     */
    handle_client(clientfd);
    close(clientfd);
  }

  return 0;
}
