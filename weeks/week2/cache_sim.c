#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int valid;
  uint64_t tag;
  uint64_t lru_tick;
} Line;

typedef struct {
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
  if (s < 0 || E <= 0 || b < 0 || s >= 20) return -1;
  c->s = s;
  c->E = E;
  c->b = b;
  c->tick = 0;
  c->hit = 0;
  c->miss = 0;
  c->eviction = 0;
  c->set_count = 1 << s;
  c->lines = (Line *)calloc((size_t)c->set_count * (size_t)E, sizeof(Line));
  return (c->lines == NULL) ? -1 : 0;
}

static void free_cache(Cache *c) {
  free(c->lines);
  c->lines = NULL;
}

static Line *line_at(Cache *c, int set, int i) {
  return &c->lines[(size_t)set * (size_t)c->E + (size_t)i];
}

static void access_cache(Cache *c, uint64_t addr) {
  uint64_t set_mask = (c->s == 0) ? 0u : ((1ull << c->s) - 1ull);
  int set = (int)((addr >> c->b) & set_mask);
  uint64_t tag = addr >> (c->b + c->s);
  int empty_idx = -1;
  int lru_idx = 0;
  uint64_t min_tick = UINT64_MAX;

  c->tick++;
  for (int i = 0; i < c->E; i++) {
    Line *ln = line_at(c, set, i);
    if (ln->valid && ln->tag == tag) {
      c->hit++;
      ln->lru_tick = c->tick;
      return;
    }
    if (!ln->valid && empty_idx < 0) {
      empty_idx = i;
    }
    if (ln->valid && ln->lru_tick < min_tick) {
      min_tick = ln->lru_tick;
      lru_idx = i;
    }
  }

  c->miss++;
  if (empty_idx >= 0) {
    Line *ln = line_at(c, set, empty_idx);
    ln->valid = 1;
    ln->tag = tag;
    ln->lru_tick = c->tick;
    return;
  }

  c->eviction++;
  Line *victim = line_at(c, set, lru_idx);
  victim->tag = tag;
  victim->lru_tick = c->tick;
}

static int process_trace(Cache *c, const char *trace_path) {
  FILE *fp = fopen(trace_path, "r");
  if (!fp) return -1;

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    char op = 0;
    uint64_t addr = 0;
    int size = 0;

    if (sscanf(line, " %c %" SCNx64 ",%d", &op, &addr, &size) < 2) {
      if (sscanf(line, " %c 0x%" SCNx64, &op, &addr) < 2) {
        continue;
      }
    }
    if (op == 'I') continue;

    if (op == 'L' || op == 'S') {
      access_cache(c, addr);
    } else if (op == 'M') {
      access_cache(c, addr);
      access_cache(c, addr);
    }
  }

  fclose(fp);
  return 0;
}

int main(int argc, char **argv) {
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

  printf("config: s=%d E=%d b=%d sets=%d\n", s, E, b, c.set_count);
  printf("result: hit=%" PRIu64 " miss=%" PRIu64 " eviction=%" PRIu64 "\n",
         c.hit, c.miss, c.eviction);

  free_cache(&c);
  return 0;
}
