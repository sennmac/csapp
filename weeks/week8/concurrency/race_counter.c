#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 所有线程共享的状态，是竞态的根源 */
struct Shared {
  long long counter;              /* 共同累加的目标 */
  long long increments_per_thread;
  bool use_lock;                  /* safe/unsafe 开关 */
  pthread_mutex_t lock;
};

static void *worker(void *arg) {
  struct Shared *shared = (struct Shared *)arg;
  long long i;

  for (i = 0; i < shared->increments_per_thread; ++i) {
    if (shared->use_lock) {
      /* safe: lock 把 load-add-store 三步打包成原子临界区 */
      pthread_mutex_lock(&shared->lock);
      shared->counter += 1;
      pthread_mutex_unlock(&shared->lock);
    } else {
      /*
       * unsafe: counter += 1 实际是三步：
       *   1. load:  从内存读 counter 到寄存器
       *   2. add:   寄存器 +1
       *   3. store: 写回内存
       * 两个线程可能同时 load 同一个旧值，各自 +1 写回，
       * 导致两次操作只生效一次 —— 这就是竞态
       */
      shared->counter += 1;
    }
  }

  return NULL;
}

int main(int argc, char **argv) {
  struct Shared shared;
  pthread_t *threads;
  int thread_count;
  long long expected;
  int i;

  if (argc != 4) {
    fprintf(stderr, "usage: %s <unsafe|safe> <threads> <increments>\n", argv[0]);
    return 1;
  }

  thread_count = atoi(argv[2]);
  if (thread_count <= 0) {
    fprintf(stderr, "thread count must be > 0\n");
    return 1;
  }

  memset(&shared, 0, sizeof(shared));
  shared.use_lock = strcmp(argv[1], "safe") == 0;
  shared.increments_per_thread = strtoll(argv[3], NULL, 10);
  pthread_mutex_init(&shared.lock, NULL);

  threads = calloc((size_t)thread_count, sizeof(*threads));
  if (threads == NULL) {
    perror("calloc");
    return 1;
  }

  struct timespec t_start, t_end;
  clock_gettime(CLOCK_MONOTONIC, &t_start);

  for (i = 0; i < thread_count; ++i) {
    pthread_create(&threads[i], NULL, worker, &shared);
  }
  for (i = 0; i < thread_count; ++i) {
    pthread_join(threads[i], NULL);
  }

  clock_gettime(CLOCK_MONOTONIC, &t_end);
  double elapsed_ms = (t_end.tv_sec - t_start.tv_sec) * 1000.0 +
                      (t_end.tv_nsec - t_start.tv_nsec) / 1e6;

  expected = (long long)thread_count * shared.increments_per_thread;
  printf("mode     : %s\n", shared.use_lock ? "safe" : "unsafe");
  printf("threads  : %d\n", thread_count);
  printf("expected : %lld\n", expected);
  printf("actual   : %lld\n", shared.counter);
  printf("delta    : %lld\n", expected - shared.counter);
  printf("time     : %.2f ms\n", elapsed_ms);

  free(threads);
  pthread_mutex_destroy(&shared.lock);
  return 0;
}
