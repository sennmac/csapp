/*
 * Week 2 cache simulator
 *
 * 这个程序不是去读取“真实 CPU 当前的 cache 状态”，
 * 而是在软件里自己搭建一个简化版 cache 模型，然后把一串内存访问 trace 喂进去，
 * 最后统计：
 *   - hit       : 命中了多少次
 *   - miss      : 缺失了多少次
 *   - eviction  : 因为 set 已满而替换了多少次
 *
 * 这份代码的目的，是把“局部性 / cache 命中”从抽象概念变成一个可以逐步推演的程序。
 *
 * 在这个模型里，一个地址会被拆成三部分：
 *
 *   [ tag | set index | block offset ]
 *
 * - block offset：地址在一个 block 里的第几个字节
 * - set index   ：这个 block 应该去 cache 的哪一个 set
 * - tag         ：到了那个 set 之后，当前 line 里装的是不是我要的那个 block
 *
 * 命令行里的三个核心参数也正好对应这三件事：
 * - s：set index 用多少位    -> 总 set 数 = 2^s
 * - E：每个 set 里多少条 line
 * - b：block offset 用多少位 -> block 大小 = 2^b 字节
 */
#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  /*
   * valid:
   *   这条 line 现在是不是已经装了一个有效 block。
   *
   * tag:
   *   当前 line 保存的是哪一个主存 block。
   *
   * lru_tick:
   *   最近一次访问这条 line 的时间戳，用来做 LRU 替换。
   */
  int valid;
  uint64_t tag;
  uint64_t lru_tick;
} Line;

typedef struct {
  /*
   * s / E / b 是 cache 的核心配置：
   *
   * s: set index 位数
   * E: 每个 set 的 line 数
   * b: block offset 位数
   *
   * tick:
   *   一个全局递增时间。每次访问时 +1，用于维护 LRU。
   *
   * hit / miss / eviction:
   *   最终统计结果。
   *
   * set_count:
   *   实际总组数，等于 2^s。
   *
   * lines:
   *   整个 cache 的所有 line。逻辑上是二维：
   *     set_count 个 set，每个 set 有 E 条 line
   *   但实现上拉平成了一维数组，访问时再手动计算索引。
   */
  int s;
  int E;
  int b;
  uint64_t tick;
  uint64_t hit;
  uint64_t miss;
  uint64_t eviction;
  int set_count;
  Line *lines;
} Cache;

static void usage(const char *prog) {
  fprintf(stderr, "usage: %s -s <set_bits> -E <lines_per_set> -b <block_bits> -t <trace>\n", prog);
}

static int init_cache(Cache *c, int s, int E, int b) {
  /*
   * 做一些基础参数检查。
   *
   * 这里的限制不是 cache 理论本身的限制，而是这个小程序为了简化实现做的约束。
   * 例如 s >= 20 时，组数会非常大，不适合这个练习程序。
   */
  if (s < 0 || E <= 0 || b < 0 || s >= 20) return -1;
  c->s = s;
  c->E = E;
  c->b = b;
  c->tick = 0;
  c->hit = 0;
  c->miss = 0;
  c->eviction = 0;

  /*
   * set 总数 = 2^s
   */
  c->set_count = 1 << s;

  /*
   * 一次性分配整个 cache 的所有 line。
   *
   * 逻辑上它像这样：
   *   set 0: line 0 ... line E-1
   *   set 1: line 0 ... line E-1
   *   ...
   *
   * calloc 会把整块内存清零，所以初始时：
   * - valid = 0
   * - tag = 0
   * - lru_tick = 0
   */
  c->lines = (Line *)calloc((size_t)c->set_count * (size_t)E, sizeof(Line));
  return (c->lines == NULL) ? -1 : 0;
}

static void free_cache(Cache *c) {
  free(c->lines);
  c->lines = NULL;
}

static Line *line_at(Cache *c, int set, int i) {
  /*
   * 把“第 set 组，第 i 条 line”映射到一维数组中的真实位置。
   *
   * 因为每组有 E 条 line，所以：
   *   set 0 占下标 [0, E-1]
   *   set 1 占下标 [E, 2E-1]
   *   ...
   *
   * 对应公式：
   *   index = set * E + i
   */
  return &c->lines[(size_t)set * (size_t)c->E + (size_t)i];
}

static void access_cache(Cache *c, uint64_t addr) {
  /*
   * 给定一个地址，模拟一次 cache access。
   *
   * 整个流程：
   * 1. 从地址中拆出 set 和 tag
   * 2. 去那个 set 里逐条查找
   * 3. 如果找到有效且 tag 相同的 line -> hit
   * 4. 如果没找到 -> miss
   *    - 有空位：直接填进去
   *    - 没空位：按 LRU 替换掉最久没用的那条
   */

  /*
   * 低 b 位是 block offset，不参与 set/tag 判断。
   * 把地址右移 b 位后，低 s 位就是 set index。
   *
   * set_mask 用来把那 s 位“抠出来”。
   *
   * 例如 s = 3 时：
   *   set_mask = 0b111
   */
  uint64_t set_mask = (c->s == 0) ? 0u : ((1ull << c->s) - 1ull);

  /*
   * set：
   *   (addr >> b) 先去掉 block offset
   *   再与 set_mask 相与，取出低 s 位
   */
  int set = (int)((addr >> c->b) & set_mask);

  /*
   * tag：
   *   再继续右移 s 位，剩下更高位就是 tag
   */
  uint64_t tag = addr >> (c->b + c->s);

  /*
   * empty_idx:
   *   如果这组里有空 line，记住第一个空位。
   *
   * lru_idx:
   *   如果这组满了，需要知道最旧的是谁，以便后续替换。
   *
   * min_tick:
   *   记录当前扫描到的最小 lru_tick。
   */
  int empty_idx = -1;
  int lru_idx = 0;
  uint64_t min_tick = UINT64_MAX;

  /*
   * 每次访问都是“新的一拍”。
   * 这样 hit 的 line 和新填入的 line 都能记录“刚刚被访问过”。
   */
  c->tick++;
  for (int i = 0; i < c->E; i++) {
    Line *ln = line_at(c, set, i);

    /*
     * 命中条件：
     * - 这条 line 有效
     * - tag 一样
     *
     * 只要满足，就说明我要的 block 已经在 cache 里。
     */
    if (ln->valid && ln->tag == tag) {
      c->hit++;

      /*
       * LRU 需要更新为“刚刚访问过”。
       */
      ln->lru_tick = c->tick;
      return;
    }

    /*
     * 如果这条 line 还没被用过，记录下来，后面 miss 时可以直接填。
     */
    if (!ln->valid && empty_idx < 0) {
      empty_idx = i;
    }

    /*
     * 同时寻找当前 set 里最久没用的有效 line。
     * 当 set 满了又 miss 时，就替换它。
     */
    if (ln->valid && ln->lru_tick < min_tick) {
      min_tick = ln->lru_tick;
      lru_idx = i;
    }
  }

  /*
   * 走到这里说明没 hit。
   */
  c->miss++;
  if (empty_idx >= 0) {
    /*
     * 这组里还有空位：
     * - 不需要 eviction
     * - 直接把新 block 填进去
     */
    Line *ln = line_at(c, set, empty_idx);
    ln->valid = 1;
    ln->tag = tag;
    ln->lru_tick = c->tick;
    return;
  }

  /*
   * 这组已满，且没 hit：
   * - miss
   * - eviction
   *
   * 替换 LRU 最旧的那条。
   */
  c->eviction++;
  Line *victim = line_at(c, set, lru_idx);
  victim->tag = tag;
  victim->lru_tick = c->tick;
}

static int process_trace(Cache *c, const char *trace_path) {
  /*
   * 读取一份 trace 文件，逐行模拟访问。
   *
   * trace 的典型格式类似：
   *   L 0x0,1
   *   S 0x18,1
   *   M 0x20,1
   *
   * 其中：
   * - L: load
   * - S: store
   * - M: modify（可理解为一次 load + 一次 store）
   * - I: instruction load（这里忽略）
   */
  FILE *fp = fopen(trace_path, "r");
  if (!fp) return -1;

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    char op = 0;
    uint64_t addr = 0;
    int size = 0;

    /*
     * 尽量兼容两种常见写法：
     *   " L 0x10,1"
     *   " L 10,1"
     *
     * 如果两种都解析失败，就跳过这行。
     */
    if (sscanf(line, " %c %" SCNx64 ",%d", &op, &addr, &size) < 2) {
      if (sscanf(line, " %c 0x%" SCNx64, &op, &addr) < 2) {
        continue;
      }
    }

    /*
     * I 表示 instruction fetch。
     * 在这个练习版模拟器里，我们只关心数据访问，所以忽略。
     */
    if (op == 'I') continue;

    if (op == 'L' || op == 'S') {
      /*
       * load 和 store 都视为一次 cache access。
       */
      access_cache(c, addr);
    } else if (op == 'M') {
      /*
       * modify 可以理解为：
       *   先读一次，再写一次
       *
       * 所以它对同一个地址要访问两次。
       *
       * 一个常见现象是：
       * - 第一次 miss
       * - 第二次因为刚刚已经装入 cache，所以 hit
       */
      access_cache(c, addr);
      access_cache(c, addr);
    }
  }

  fclose(fp);
  return 0;
}

int main(int argc, char **argv) {
  /*
   * 命令行参数：
   * -s <set_bits>
   * -E <lines_per_set>
   * -b <block_bits>
   * -t <trace_path>
   */
  int s = -1;
  int E = -1;
  int b = -1;
  const char *trace = NULL;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-s") && i + 1 < argc) {
      s = atoi(argv[++i]);
    } else if (!strcmp(argv[i], "-E") && i + 1 < argc) {
      E = atoi(argv[++i]);
    } else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
      b = atoi(argv[++i]);
    } else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
      trace = argv[++i];
    } else {
      usage(argv[0]);
      return 2;
    }
  }

  /*
   * 参数不完整或非法，打印 usage。
   *
   * 这里返回 2，表示“命令行用法错误”。
   */
  if (s < 0 || E <= 0 || b < 0 || !trace) {
    usage(argv[0]);
    return 2;
  }

  Cache c;
  if (init_cache(&c, s, E, b) != 0) {
    fprintf(stderr, "cache init failed\n");
    return 1;
  }

  if (process_trace(&c, trace) != 0) {
    fprintf(stderr, "failed to open trace: %s\n", trace);
    free_cache(&c);
    return 1;
  }

  /*
   * 打印最终配置与统计结果。
   */
  printf("config: s=%d E=%d b=%d sets=%d\n", s, E, b, c.set_count);
  printf("result: hit=%" PRIu64 " miss=%" PRIu64 " eviction=%" PRIu64 "\n",
         c.hit, c.miss, c.eviction);

  free_cache(&c);
  return 0;
}
