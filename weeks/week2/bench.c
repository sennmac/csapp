#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Simple micro-bench: compare row-major vs col-major and a blocked transpose.
// Not a perfect cache benchmark, but good for building locality intuition.

static inline uint64_t ns_since(const struct timespec *a, const struct timespec *b) {
  return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static void *xmalloc(size_t n) {
  void *p = NULL;
  // 64-byte align helps avoid weirdness.
  if (posix_memalign(&p, 64, n) != 0) return NULL;
  return p;
}

static void fill(float *a, int n, float seed) {
  for (int i = 0; i < n; i++) a[i] = seed + (float)(i % 97);
}

static float checksum(const float *a, int n) {
  double s = 0.0;
  for (int i = 0; i < n; i++) s += a[i];
  return (float)s;
}

static void sum_row_major(const float *A, int N, volatile double *out) {
  double s = 0.0;
  for (int i = 0; i < N; i++) {
    const float *row = A + (size_t)i * (size_t)N;
    for (int j = 0; j < N; j++) s += row[j];
  }
  *out = s;
}

static void sum_col_major(const float *A, int N, volatile double *out) {
  double s = 0.0;
  for (int j = 0; j < N; j++) {
    for (int i = 0; i < N; i++) s += A[(size_t)i * (size_t)N + (size_t)j];
  }
  *out = s;
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

static uint64_t time_sum(void (*fn)(const float*, int, volatile double*), const float *A, int N, int iters, volatile double *sink) {
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) fn(A, N, sink);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  return ns_since(&t0, &t1);
}

static uint64_t time_transpose(void (*fn)(const float*, float*, int), const float *A, float *B, int N, int iters) {
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) fn(A, B, N);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  return ns_since(&t0, &t1);
}

int main(int argc, char **argv) {
  int N = (argc >= 2) ? atoi(argv[1]) : 2048;
  int iters = (argc >= 3) ? atoi(argv[2]) : 5;
  int BS = (argc >= 4) ? atoi(argv[3]) : 32;
  if (N <= 0 || iters <= 0 || BS <= 0) {
    fprintf(stderr, "usage: %s [N=2048] [iters=5] [BS=32]\n", argv[0]);
    return 2;
  }

  size_t bytes = (size_t)N * (size_t)N * sizeof(float);
  float *A = (float*)xmalloc(bytes);
  float *B = (float*)xmalloc(bytes);
  if (!A || !B) {
    fprintf(stderr, "alloc failed (%zu bytes each)\n", bytes);
    return 1;
  }

  fill(A, (int)((size_t)N*(size_t)N), 1.0f);
  memset(B, 0, bytes);

  volatile double sink = 0.0;

  // Warm up caches / pages
  sink += checksum(A, (int)((size_t)N*(size_t)N));

  uint64_t t_row = time_sum(sum_row_major, A, N, iters, &sink);
  uint64_t t_col = time_sum(sum_col_major, A, N, iters, &sink);

  uint64_t t_tn = time_transpose(transpose_naive, A, B, N, iters);

  // blocked transpose timing (wrap into matching signature)
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) transpose_blocked(A, B, N, BS);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_tb = ns_since(&t0, &t1);

  double elems = (double)N * (double)N;
  printf("N=%d iters=%d BS=%d\n", N, iters, BS);
  printf("sum row-major:  %.3f ms (%.2f ns/elem)\n", (double)t_row/1e6, (double)t_row/(iters*elems));
  printf("sum col-major:  %.3f ms (%.2f ns/elem)\n", (double)t_col/1e6, (double)t_col/(iters*elems));
  printf("transpose naive:   %.3f ms\n", (double)t_tn/1e6);
  printf("transpose blocked: %.3f ms\n", (double)t_tb/1e6);
  printf("(sink=%f)\n", (double)sink);

  free(A);
  free(B);
  return 0;
}
