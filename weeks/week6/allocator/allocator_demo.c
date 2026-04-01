#include "allocator.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_LINE = 256, SLOT_COUNT = 52 };

struct Slot {
  void *ptr;
};

static int slot_index(const char *name) {
  unsigned char c = (unsigned char)name[0];

  if (name[0] == '\0' || name[1] != '\0') {
    return -1;
  }
  if (c >= 'a' && c <= 'z') {
    return c - 'a';
  }
  if (c >= 'A' && c <= 'Z') {
    return 26 + (c - 'A');
  }
  return -1;
}

static int run_trace(const char *trace_path) {
  FILE *fp = fopen(trace_path, "r");
  char line[MAX_LINE];
  struct Slot slots[SLOT_COUNT];

  if (fp == NULL) {
    perror(trace_path);
    return 1;
  }

  memset(slots, 0, sizeof(slots));

  while (fgets(line, sizeof(line), fp) != NULL) {
    char *cmd;
    char *arg1;
    char *arg2;
    char *save = NULL;

    if (line[0] == '#' || isspace((unsigned char)line[0])) {
      continue;
    }

    cmd = strtok_r(line, " \t\r\n", &save);
    if (cmd == NULL) {
      continue;
    }

    if (strcmp(cmd, "alloc") == 0) {
      int idx;
      size_t size;

      arg1 = strtok_r(NULL, " \t\r\n", &save);
      arg2 = strtok_r(NULL, " \t\r\n", &save);
      if (arg1 == NULL || arg2 == NULL) {
        fprintf(stderr, "alloc requires a slot and size\n");
        fclose(fp);
        return 1;
      }

      idx = slot_index(arg1);
      size = (size_t)strtoull(arg2, NULL, 10);
      if (idx < 0) {
        fprintf(stderr, "invalid slot name: %s\n", arg1);
        fclose(fp);
        return 1;
      }

      slots[idx].ptr = lab_malloc(size);
      printf("alloc %-2s size=%zu -> %p\n", arg1, size, slots[idx].ptr);
    } else if (strcmp(cmd, "free") == 0) {
      int idx;

      arg1 = strtok_r(NULL, " \t\r\n", &save);
      if (arg1 == NULL) {
        fprintf(stderr, "free requires a slot\n");
        fclose(fp);
        return 1;
      }

      idx = slot_index(arg1);
      if (idx < 0) {
        fprintf(stderr, "invalid slot name: %s\n", arg1);
        fclose(fp);
        return 1;
      }

      printf("free  %-2s ptr=%p\n", arg1, slots[idx].ptr);
      lab_free(slots[idx].ptr);
      slots[idx].ptr = NULL;
    } else if (strcmp(cmd, "dump") == 0) {
      puts("-- dump --");
      lab_dump();
    } else {
      fprintf(stderr, "unknown command: %s\n", cmd);
      fclose(fp);
      return 1;
    }
  }

  fclose(fp);
  return 0;
}

int main(int argc, char **argv) {
  const char *trace_path;
  size_t arena_size = 256;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: %s <trace> [arena_size]\n", argv[0]);
    return 1;
  }

  trace_path = argv[1];
  if (argc == 3) {
    arena_size = (size_t)strtoull(argv[2], NULL, 10);
  }

  if (lab_init(arena_size) != 0) {
    fprintf(stderr, "invalid arena size: %zu\n", arena_size);
    return 1;
  }

  return run_trace(trace_path);
}
