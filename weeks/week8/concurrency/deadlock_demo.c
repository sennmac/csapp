#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Barrier {
  pthread_mutex_t lock;
  pthread_cond_t cv;
  int arrived;
  int target;
};

struct WorkerArgs {
  const char *name;
  pthread_mutex_t *first;
  pthread_mutex_t *second;
  struct Barrier *barrier;
  bool ordered_mode;
};

static void barrier_init(struct Barrier *barrier, int target) {
  pthread_mutex_init(&barrier->lock, NULL);
  pthread_cond_init(&barrier->cv, NULL);
  barrier->arrived = 0;
  barrier->target = target;
}

static void barrier_destroy(struct Barrier *barrier) {
  pthread_mutex_destroy(&barrier->lock);
  pthread_cond_destroy(&barrier->cv);
}

static void barrier_wait(struct Barrier *barrier) {
  pthread_mutex_lock(&barrier->lock);
  barrier->arrived += 1;
  if (barrier->arrived == barrier->target) {
    pthread_cond_broadcast(&barrier->cv);
  } else {
    while (barrier->arrived < barrier->target) {
      pthread_cond_wait(&barrier->cv, &barrier->lock);
    }
  }
  pthread_mutex_unlock(&barrier->lock);
}

static void lock_in_address_order(pthread_mutex_t *a, pthread_mutex_t *b) {
  pthread_mutex_t *first = a < b ? a : b;
  pthread_mutex_t *second = a < b ? b : a;

  pthread_mutex_lock(first);
  pthread_mutex_lock(second);
}

static void unlock_in_address_order(pthread_mutex_t *a, pthread_mutex_t *b) {
  pthread_mutex_t *first = a < b ? a : b;
  pthread_mutex_t *second = a < b ? b : a;

  pthread_mutex_unlock(second);
  pthread_mutex_unlock(first);
}

static void *worker_main(void *arg) {
  struct WorkerArgs *args = (struct WorkerArgs *)arg;

  if (args->ordered_mode) {
    printf("%s locking in global order\n", args->name);
    lock_in_address_order(args->first, args->second);
    printf("%s entered critical section safely\n", args->name);
    usleep(100000);
    unlock_in_address_order(args->first, args->second);
    return NULL;
  }

  pthread_mutex_lock(args->first);
  printf("%s locked first mutex\n", args->name);
  barrier_wait(args->barrier);

  for (int attempt = 0; attempt < 5; ++attempt) {
    if (pthread_mutex_trylock(args->second) == 0) {
      printf("%s unexpectedly acquired both locks\n", args->name);
      pthread_mutex_unlock(args->second);
      pthread_mutex_unlock(args->first);
      return NULL;
    }

    printf("%s waiting for second mutex (attempt %d)\n",
           args->name,
           attempt + 1);
    usleep(120000);
  }

  printf("%s observed a deadlock pattern: hold-and-wait + circular wait\n",
         args->name);
  pthread_mutex_unlock(args->first);
  return NULL;
}

int main(int argc, char **argv) {
  pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;
  struct Barrier barrier;
  struct WorkerArgs a_args;
  struct WorkerArgs b_args;
  pthread_t thread_a;
  pthread_t thread_b;
  bool ordered_mode;

  if (argc != 2 || (strcmp(argv[1], "deadlock") != 0 && strcmp(argv[1], "ordered") != 0)) {
    fprintf(stderr, "usage: %s <deadlock|ordered>\n", argv[0]);
    return 1;
  }

  ordered_mode = strcmp(argv[1], "ordered") == 0;
  barrier_init(&barrier, 2);

  a_args.name = "worker-A";
  a_args.first = &lock_a;
  a_args.second = &lock_b;
  a_args.barrier = &barrier;
  a_args.ordered_mode = ordered_mode;

  b_args.name = "worker-B";
  b_args.first = &lock_b;
  b_args.second = &lock_a;
  b_args.barrier = &barrier;
  b_args.ordered_mode = ordered_mode;

  pthread_create(&thread_a, NULL, worker_main, &a_args);
  pthread_create(&thread_b, NULL, worker_main, &b_args);

  pthread_join(thread_a, NULL);
  pthread_join(thread_b, NULL);

  barrier_destroy(&barrier);
  return 0;
}
