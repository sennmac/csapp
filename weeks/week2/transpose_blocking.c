#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline uint64_t ns_since(const struct timespec *a, const struct timespec *b) {
  return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static void *xmalloc(size_t n) {
  void *p = NULL;
  if (posix_memalign(&p, 64, n) != 0) return NULL;
  return p;
}

static void fill(float *a, int n) {
  for (int i = 0; i < n; i++) a[i] = (float)(i % 113);
}

static void transpose_naive(const float *A, float *B, int N) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
    }
  }
}

static void transpose_blocked(const float *A, float *B, int N, int BS) {
  for (int i0 = 0; i0 < N; i0 += BS) {
    for (int j0 = 0; j0 < N; j0 += BS) {
      int imax = (i0 + BS < N) ? (i0 + BS) : N;
      int jmax = (j0 + BS < N) ? (j0 + BS) : N;
      for (int i = i0; i < imax; i++) {
        for (int j = j0; j < jmax; j++) {
          B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
        }
      }
    }
  }
}

static float checksum(const float *a, int n) {
  double s = 0.0;
  for (int i = 0; i < n; i++) s += a[i];
  return (float)s;
}

int main(int argc, char **argv) {
  int N = (argc >= 2) ? atoi(argv[1]) : 1024;
  int iters = (argc >= 3) ? atoi(argv[2]) : 5;
  int BS = (argc >= 4) ? atoi(argv[3]) : 16;
  if (N <= 0 || iters <= 0 || BS <= 0) {
    fprintf(stderr, "usage: %s [N=1024] [iters=5] [BS=16]\n", argv[0]);
    return 2;
  }

  size_t bytes = (size_t)N * (size_t)N * sizeof(float);
  float *A = (float *)xmalloc(bytes);
  float *B = (float *)xmalloc(bytes);
  if (!A || !B) {
    fprintf(stderr, "alloc failed\n");
    free(A);
    free(B);
    return 1;
  }

  fill(A, (int)((size_t)N * (size_t)N));

  struct timespec t0, t1;

  memset(B, 0, bytes);
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_naive(A, B, N);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_naive = ns_since(&t0, &t1);
  float c1 = checksum(B, (int)((size_t)N * (size_t)N));

  memset(B, 0, bytes);
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_blocked(A, B, N, BS);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_blocked = ns_since(&t0, &t1);
  float c2 = checksum(B, (int)((size_t)N * (size_t)N));

  printf("transpose compare N=%d iters=%d BS=%d\n", N, iters, BS);
  printf("naive:   %.3f ms total (%.3f ms/iter)\n", (double)t_naive / 1e6, (double)t_naive / 1e6 / iters);
  printf("blocked: %.3f ms total (%.3f ms/iter)\n", (double)t_blocked / 1e6, (double)t_blocked / 1e6 / iters);
  if (t_blocked > 0) {
    printf("speedup: %.2fx\n", (double)t_naive / (double)t_blocked);
  }
  printf("checksum naive=%.1f blocked=%.1f\n", (double)c1, (double)c2);

  free(A);
  free(B);
  return 0;
}
