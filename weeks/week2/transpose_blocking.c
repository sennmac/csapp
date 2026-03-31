#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Day 4 blocked transpose
 *
 * Purpose:
 * - Compare naive transpose vs blocked transpose under the same workload.
 * - Show how tiling improves locality and reduces effective memory traffic.
 *
 * Key idea:
 * - Naive transpose has strided stores on B (poor locality).
 * - Blocking limits working set to a BS x BS tile, so reuse happens sooner.
 */

static inline uint64_t ns_since(const struct timespec *a, const struct timespec *b) {
  /* Monotonic timestamp delta in nanoseconds. */
  return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static void *xmalloc(size_t n) {
  void *p = NULL;
  /* 64-byte alignment helps benchmark stability. */
  if (posix_memalign(&p, 64, n) != 0) return NULL;
  return p;
}

static void fill(float *a, int n) {
  /* Deterministic initialization for reproducible results. */
  for (int i = 0; i < n; i++) a[i] = (float)(i % 113);
}

static void transpose_naive(const float *A, float *B, int N) {
  /*
   * Baseline implementation:
   *   B[j][i] = A[i][j]
   *
   * Read A: contiguous in inner loop.
   * Write B: stride = N * sizeof(float) in inner loop.
   * For large N this usually hurts cache efficiency.
   */
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
    }
  }
}

static void transpose_blocked(const float *A, float *B, int N, int BS) {
  /*
   * Blocked (tiled) transpose:
   * - Outer loops choose one BS x BS tile.
   * - Inner loops transpose only inside that tile.
   *
   * Why this helps:
   * - A tile and B tile are much smaller than the full matrix.
   * - Data touched "soon again" is more likely to remain in cache.
   * - Write path on B still has stride in the innermost loop, but
   *   reuse distance is reduced by confining execution to a tile.
   */
  for (int i0 = 0; i0 < N; i0 += BS) {
    for (int j0 = 0; j0 < N; j0 += BS) {
      /*
       * Clamp tile boundaries for tail tiles when N is not divisible by BS.
       * This keeps the implementation correct for all positive N and BS.
       */
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
  /* Compare outputs from naive and blocked quickly. */
  double s = 0.0;
  for (int i = 0; i < n; i++) s += a[i];
  return (float)s;
}

int main(int argc, char **argv) {
  /*
   * CLI:
   *   argv[1] -> N     (default 1024)
   *   argv[2] -> iters (default 5)
   *   argv[3] -> BS    (default 16)
   *
   * BS tuning notes:
   * - Too small: more loop overhead, less benefit from each tile.
   * - Too large: tile working set exceeds cache-friendly range.
   */
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

  /*
   * Measure naive first.
   * memset keeps both paths comparable and avoids stale data effects.
   */
  memset(B, 0, bytes);
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_naive(A, B, N);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_naive = ns_since(&t0, &t1);
  float c1 = checksum(B, (int)((size_t)N * (size_t)N));

  /* Measure blocked under the same N/iters/data. */
  memset(B, 0, bytes);
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_blocked(A, B, N, BS);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_blocked = ns_since(&t0, &t1);
  float c2 = checksum(B, (int)((size_t)N * (size_t)N));

  /*
   * speedup > 1 means blocked is faster.
   * checksum equality is a quick sanity check for functional equivalence.
   */
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
