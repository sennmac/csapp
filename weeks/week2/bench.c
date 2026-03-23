#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Week 2 micro-benchmark
 *
 * 这个程序不是为了做一个“特别严谨、可发表论文”的 benchmark，
 * 而是为了用尽量少的代码，把下面几件事直观地跑出来：
 *
 * 1. 同样是把矩阵里所有元素访问一遍，按行访问和按列访问为什么会快慢不同。
 * 2. 同样是做矩阵转置，朴素版本和 blocked 版本为什么性能会差很多。
 * 3. 代码里的循环顺序，是如何影响 cache 局部性的。
 *
 * 这里矩阵虽然概念上是 N x N 的二维数组，但实现上我们把它当成一段连续的一维内存。
 * 这样做有两个好处：
 * - 可以在运行时按参数决定 N，不需要写死数组大小。
 * - 更容易把“二维下标”直接对应到“底层线性地址”。
 *
 * 对于 C 语言，二维数组在内存里的默认布局是 row-major（按行连续）。
 * 也就是说：
 *   A[0][0], A[0][1], A[0][2] ... A[0][N-1]
 * 是连续排在一起的；
 * 然后才轮到下一行：
 *   A[1][0], A[1][1], ...
 *
 * 因此：
 * - 内层循环如果沿着列 j 递增，就更容易连续访问内存，空间局部性好。
 * - 内层循环如果沿着行 i 递增，而列固定，就会形成大步长跳跃访问，cache 更难受益。
 */

static inline uint64_t ns_since(const struct timespec *a, const struct timespec *b) {
  /*
   * 计算两个时间点之间的纳秒差。
   *
   * 这里使用 CLOCK_MONOTONIC 的结果做差。选 monotonic 的原因是：
   * - 它不会被系统时间调整影响；
   * - 更适合做性能测量。
   *
   * 返回值统一用 uint64_t，避免大矩阵、多次迭代时精度不够。
   */
  return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ull + (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static void *xmalloc(size_t n) {
  void *p = NULL;
  /*
   * 用 64-byte 对齐分配内存。
   *
   * 这不是程序正确性的要求，而是一个实验层面的选择：
   * - 64 字节通常接近常见 cache line 大小；
   * - 对齐后可以减少一些“起始地址很别扭”带来的噪音；
   * - 让不同实验之间的结果更稳定一些。
   *
   * 如果分配失败，返回 NULL，由调用者处理。
   */
  if (posix_memalign(&p, 64, n) != 0) return NULL;
  return p;
}

static void fill(float *a, int n, float seed) {
  /*
   * 给数组填充可重复的数据模式。
   *
   * 这里不是随机数，而是一个简单的周期序列：
   *   seed + (i % 97)
   *
   * 这样做的目的：
   * - 保证每次运行的数据是确定的，便于复现实验；
   * - 避免整块全 0 这种过于“特殊”的数据；
   * - 让编译器更难把后续访问当成可完全省略的无意义操作。
   */
  for (int i = 0; i < n; i++) a[i] = seed + (float)(i % 97);
}

static float checksum(const float *a, int n) {
  /*
   * 对整个数组做一次求和，返回一个简单的校验值。
   *
   * 在这个文件里，它主要有两个作用：
   * 1. warm-up：先把数组对应的内存页摸一遍，减少首次缺页/首次访问噪音。
   * 2. 防止“看起来用了数组，其实结果完全没被消费”的情况。
   *
   * 这里累加变量用 double，是为了比 float 稍微稳一点。
   * 最后再转回 float，因为我们只把它当成一个轻量的校验/副作用载体。
   */
  double s = 0.0;
  for (int i = 0; i < n; i++) s += a[i];
  return (float)s;
}

static void sum_row_major(const float *A, int N, volatile double *out) {
  /*
   * 按行访问矩阵并求和。
   *
   * 逻辑上的二维形式：
   *   for each row i
   *     for each col j
   *       sum += A[i][j]
   *
   * 由于 C 的矩阵按行连续存储，所以当 i 固定、j 递增时，
   * A[i][0], A[i][1], A[i][2] ... 在内存里通常是连续的。
   *
   * 这意味着：
   * - 每次读入一条 cache line，后续几个元素很可能都在这条 line 里；
   * - 空间局部性很好；
   * - 对硬件预取器也更友好。
   *
   * 这就是“row-major 遍历更顺着内存布局”的具体体现。
   */
  double s = 0.0;
  for (int i = 0; i < N; i++) {
    /*
     * 先拿到第 i 行的起始地址。
     *
     * A 本质上是一段连续的一维内存；
     * 第 i 行的首元素在线性地址中的偏移是 i * N。
     *
     * 后面用 row[j] 访问，比每次都写 A[i*N + j] 更清楚，
     * 也少做一点重复地址表达式。
     */
    const float *row = A + (size_t)i * (size_t)N;
    for (int j = 0; j < N; j++) s += row[j];
  }
  /*
   * 把结果写到 volatile 指向的位置，而不是直接丢掉。
   *
   * benchmark 中这很常见：如果最终结果完全没被用，
   * 编译器可能会认为这整段循环“没有可观察副作用”，进而优化掉。
   *
   * 写到 volatile 目标上，相当于告诉编译器：
   * “这个值对外部是可见的，别随便删。”
   */
  *out = s;
}

static void sum_col_major(const float *A, int N, volatile double *out) {
  /*
   * 按列访问矩阵并求和。
   *
   * 逻辑上的二维形式：
   *   for each col j
   *     for each row i
   *       sum += A[i][j]
   *
   * 注意这和上面的 row-major 访问在“数学上”做的是同一件事：
   * 都是把每个元素加一遍。
   *
   * 但它们的访存顺序完全不同。
   *
   * 这里 j 固定、i 递增时，访问地址是：
   *   A[0][j], A[1][j], A[2][j], ...
   *
   * 在线性内存里，这些位置之间相隔大约 N 个 float。
   * 如果 N 很大，就会形成“大步长跳跃访问”：
   * - 一条 cache line 里只用到很少几个元素；
   * - 刚加载进来的邻近数据，短时间内不一定再访问；
   * - 空间局部性明显更差。
   *
   * 所以这段代码特别适合拿来和 sum_row_major 做对照。
   */
  double s = 0.0;
  for (int j = 0; j < N; j++) {
    for (int i = 0; i < N; i++) s += A[(size_t)i * (size_t)N + (size_t)j];
  }
  *out = s;
}

static void transpose_naive(const float *A, float *B, int N) {
  /*
   * 最朴素的矩阵转置：
   *   B[j][i] = A[i][j]
   *
   * 逻辑上非常直观，也最容易先写对。
   *
   * 但性能上它有一个经典问题：
   * - 读 A[i][j] 时，j 在内层，读取 A 是连续的，比较友好。
   * - 写 B[j][i] 时，j 在内层，写入 B 却变成了“跨行跳写”。
   *
   * 也就是说，这段代码的瓶颈往往不在算术，而在写 B 时的局部性差。
   */
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
    }
  }
}

static void transpose_blocked(const float *A, float *B, int N, int BS) {
  /*
   * blocked / tiled transpose：按小块做转置，而不是整张矩阵一口气扫完。
   *
   * 核心思想：
   *   把大矩阵切成很多 BS x BS 的小块，
   *   一次只处理一个小块。
   *
   * 这么做的原因是：
   * - 小块的数据量更容易落在 cache 中；
   * - 在处理一个块期间，A 和 B 的访问区域都更集中；
   * - 可以显著改善局部性，尤其是写 B 的行为。
   *
   * 这是一种非常经典的 cache 优化手法。
   */
  for (int i0 = 0; i0 < N; i0 += BS) {
    for (int j0 = 0; j0 < N; j0 += BS) {
      /*
       * 当前块的实际边界。
       *
       * 如果 N 不是 BS 的整数倍，最后一排/最后一列块可能会“残缺”。
       * 因此不能直接写到 i0 + BS、j0 + BS 为止，
       * 而是要和 N 取较小值，避免越界。
       */
      int imax = (i0 + BS < N) ? (i0 + BS) : N;
      int jmax = (j0 + BS < N) ? (j0 + BS) : N;

      /*
       * 下面两层循环只处理当前这个块内部的元素。
       *
       * 你可以把它想成：
       * - 外层两层负责“选块”
       * - 内层两层负责“在块内完成普通转置”
       */
      for (int i = i0; i < imax; i++) {
        for (int j = j0; j < jmax; j++) {
          B[(size_t)j * (size_t)N + (size_t)i] = A[(size_t)i * (size_t)N + (size_t)j];
        }
      }
    }
  }
}

static uint64_t time_sum(void (*fn)(const float*, int, volatile double*), const float *A, int N, int iters, volatile double *sink) {
  /*
   * 对“矩阵求和类函数”做统一计时。
   *
   * 把被测函数作为函数指针传进来，这样 row-major 和 col-major
   * 就可以复用同一套计时逻辑，减少重复代码，也让对比更直接。
   */
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) fn(A, N, sink);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  return ns_since(&t0, &t1);
}

static uint64_t time_transpose(void (*fn)(const float*, float*, int), const float *A, float *B, int N, int iters) {
  /*
   * 对“转置函数（签名固定为 A, B, N）”做统一计时。
   *
   * 这里能直接复用同一套包装，是因为 transpose_naive 的参数恰好是：
   *   (const float *A, float *B, int N)
   *
   * blocked 版本多了一个 BS 参数，所以后面 main 里单独计时。
   */
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) fn(A, B, N);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  return ns_since(&t0, &t1);
}

int main(int argc, char **argv) {
  /*
   * 命令行参数：
   *   argv[1] -> N     : 矩阵维度，默认 2048
   *   argv[2] -> iters : 每个实验重复次数，默认 5
   *   argv[3] -> BS    : blocked transpose 的块大小，默认 32
   *
   * N 越大，矩阵越容易超过 cache 容量，访问模式差异也通常会更明显。
   * iters 用来把单次测量放大一些，降低计时抖动。
   */
  int N = (argc >= 2) ? atoi(argv[1]) : 2048;
  int iters = (argc >= 3) ? atoi(argv[2]) : 5;
  int BS = (argc >= 4) ? atoi(argv[3]) : 32;
  if (N <= 0 || iters <= 0 || BS <= 0) {
    fprintf(stderr, "usage: %s [N=2048] [iters=5] [BS=32]\n", argv[0]);
    return 2;
  }

  /*
   * 一个 N x N 的 float 矩阵，总字节数是 N * N * sizeof(float)。
   *
   * 这里会分配两份：
   * - A: 输入矩阵
   * - B: 转置输出矩阵
   */
  size_t bytes = (size_t)N * (size_t)N * sizeof(float);
  float *A = (float*)xmalloc(bytes);
  float *B = (float*)xmalloc(bytes);
  if (!A || !B) {
    fprintf(stderr, "alloc failed (%zu bytes each)\n", bytes);
    return 1;
  }

  /*
   * 初始化实验数据：
   * - A 填入确定内容
   * - B 清零，避免读取未初始化内存
   */
  fill(A, (int)((size_t)N*(size_t)N), 1.0f);
  memset(B, 0, bytes);

  /*
   * sink 是 benchmark 中常见的“黑洞变量”：
   * - 用来接住计算结果；
   * - 防止编译器把整段计算优化掉。
   */
  volatile double sink = 0.0;

  /*
   * 先对 A 做一次 checksum，作为 warm-up。
   *
   * 这里的 warm-up 更多是在做两件事：
   * - 提前把相关内存页映射进来；
   * - 让首次访问的额外开销不要完全污染后面的正式测量。
   *
   * 注意：这并不代表 benchmark 已经“绝对公平”，
   * 只是做了一个很基础的预热。
   */
  sink += checksum(A, (int)((size_t)N*(size_t)N));

  /*
   * 实验 1：比较按行求和和按列求和。
   *
   * 数学工作量基本一样，差别主要体现在访问顺序。
   */
  uint64_t t_row = time_sum(sum_row_major, A, N, iters, &sink);
  uint64_t t_col = time_sum(sum_col_major, A, N, iters, &sink);

  /*
   * 实验 2：测最朴素转置。
   */
  uint64_t t_tn = time_transpose(transpose_naive, A, B, N, iters);

  /*
   * 实验 3：测 blocked transpose。
   *
   * 由于 blocked 版本多了 BS 参数，无法直接塞进上面的
   * time_transpose(fn, A, B, N, iters) 这一套固定签名里，
   * 所以这里单独写一段计时逻辑。
   */
  struct timespec t0, t1;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int k = 0; k < iters; k++) transpose_blocked(A, B, N, BS);
  clock_gettime(CLOCK_MONOTONIC, &t1);
  uint64_t t_tb = ns_since(&t0, &t1);

  /*
   * elems 表示一个矩阵包含多少个元素。
   * 打印 ns/elem 是为了把“总时间”换算成更容易横向比较的单位成本。
   */
  double elems = (double)N * (double)N;
  printf("N=%d iters=%d BS=%d\n", N, iters, BS);
  printf("sum row-major:  %.3f ms (%.2f ns/elem)\n", (double)t_row/1e6, (double)t_row/(iters*elems));
  printf("sum col-major:  %.3f ms (%.2f ns/elem)\n", (double)t_col/1e6, (double)t_col/(iters*elems));
  printf("transpose naive:   %.3f ms\n", (double)t_tn/1e6);
  printf("transpose blocked: %.3f ms\n", (double)t_tb/1e6);

  /*
   * 打印 sink，进一步确保编译器认为前面的结果确实被程序使用了。
   * 这不是业务输出，而是 benchmark 的一个防优化技巧。
   */
  printf("(sink=%f)\n", (double)sink);

  free(A);
  free(B);
  return 0;
}
