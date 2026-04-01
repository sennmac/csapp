#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Shared {
  long long counter;
  long long increments_per_thread;
  bool use_lock;
  pthread_mutex_t lock;
};

static void *worker(void *arg) {
  struct Shared *shared = (struct Shared *)arg;
  long long i;

  for (i = 0; i < shared->increments_per_thread; ++i) {
    if (shared->use_lock) {
      pthread_mutex_lock(&shared->lock);
      shared->counter += 1;
      pthread_mutex_unlock(&shared->lock);
    } else {
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

  for (i = 0; i < thread_count; ++i) {
    pthread_create(&threads[i], NULL, worker, &shared);
  }
  for (i = 0; i < thread_count; ++i) {
    pthread_join(threads[i], NULL);
  }

  expected = (long long)thread_count * shared.increments_per_thread;
  printf("mode     : %s\n", shared.use_lock ? "safe" : "unsafe");
  printf("threads  : %d\n", thread_count);
  printf("expected : %lld\n", expected);
  printf("actual   : %lld\n", shared.counter);
  printf("delta    : %lld\n", expected - shared.counter);

  free(threads);
  pthread_mutex_destroy(&shared.lock);
  return 0;
}
