#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Day 3 baseline transpose
 *
 * Purpose:
 * - Build a "naive transpose" performance baseline before introducing blocking.
 * - Keep implementation simple so bottlenecks are easy to reason about.
 *
 * Core observation to verify:
 * - Read path (A[i][j]) is contiguous when j is the inner loop.
 * - Write path (B[j][i]) jumps by N * sizeof(float), which hurts locality.
 */

static inline uint64_t ns_since(const struct timespec *a, const struct timespec *b) {
  /* Monotonic timestamp delta in nanoseconds. */
  return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static void *xmalloc(size_t n) {
  void *p = NULL;
  /*
   * 64-byte alignment helps reduce alignment noise in memory benchmarks.
   * It is not required for correctness.
   */
  if (posix_memalign(&p, 64, n) != 0) return NULL;
  return p;
}

static void fill(float *a, int n) {
  /* Deterministic data pattern for reproducible runs. */
  for (int i = 0; i < n; i++) a[i] = (float)(i % 97);
}

static void transpose_naive(const float *A, float *B, int N) {
  /*
   * Naive transpose:
   *   B[j][i] = A[i][j]
   *
   * Access pattern:
   * - A read: contiguous in the inner loop (good locality).
   * - B write: strided in the inner loop (poor locality).
   *
   * For N=1024 and float=4B:
   * - Store stride on B is 1024 * 4 = 4096 bytes.
   * - That often means each store touches a different cache line.
   * - Under write-back + write-allocate, this can amplify memory traffic.
   */
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
    }
  }
}

static float checksum(const float *a, int n) {
  /*
   * Lightweight correctness guard:
   * if checksum is stable across runs, output is likely consistent.
   */
  double s = 0.0;
  for (int i = 0; i < n; i++) s += a[i];
  return (float)s;
}

int main(int argc, char **argv) {
  /*
   * CLI:
   *   argv[1] -> N     (matrix dimension, default 1024)
   *   argv[2] -> iters (repeat count, default 5)
   */
  int N = (argc >= 2) ? atoi(argv[1]) : 1024;
  int iters = (argc >= 3) ? atoi(argv[2]) : 5;
  if (N <= 0 || iters <= 0) {
    fprintf(stderr, "usage: %s [N=1024] [iters=5]\n", argv[0]);
    return 2;
  }

  /* Allocate two N x N matrices in row-major linear storage. */
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
  memset(B, 0, bytes);

  /* Time only the transpose loop body. */
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_naive(A, B, N);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);

  /*
   * Report both total and normalized cost.
   * ns/elem is convenient for comparing across N and iters.
   */
  uint64_t ns = ns_since(&t0, &t1);
  double elems = (double)N * (double)N * (double)iters;
  printf("baseline transpose N=%d iters=%d\n", N, iters);
  printf("total: %.3f ms  avg: %.3f ms/iter  %.2f ns/elem\n",
         (double)ns / 1e6, (double)ns / 1e6 / iters, (double)ns / elems);
  printf("checksum(B)=%.1f\n", (double)checksum(B, (int)((size_t)N * (size_t)N)));

  free(A);
  free(B);
  return 0;
}
