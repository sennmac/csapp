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
  for (int i = 0; i < n; i++) a[i] = (float)(i % 97);
}

static void transpose_naive(const float *A, float *B, int N) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
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
  if (N <= 0 || iters <= 0) {
    fprintf(stderr, "usage: %s [N=1024] [iters=5]\n", argv[0]);
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
  memset(B, 0, bytes);

  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) {
    transpose_naive(A, B, N);
  }
  clock_gettime(CLOCK_MONOTONIC, &t1);

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
