#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_LINE = 128 };

static void explode_bomb(const char *why) {
  fprintf(stderr, "BOOM! %s\n", why);
  exit(1);
}

static void phase_defused(int phase) {
  printf("Phase %d defused.\n", phase);
}

static int is_blank_line(const char *s) {
  while (*s != '\0') {
    if (!isspace((unsigned char)*s)) return 0;
    s++;
  }
  return 1;
}

static void read_nonempty_line(FILE *fp, char *buf, size_t size) {
  while (fgets(buf, (int)size, fp) != NULL) {
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    if (!is_blank_line(buf)) return;
  }
  explode_bomb("unexpected end of input");
}

static void phase_1(const char *input) {
  static const unsigned char key[6] = {0x11u, 0x23u, 0x07u, 0x31u, 0x19u, 0x2au};
  static const unsigned char target[6] = {0x77u, 0x51u, 0x66u, 0x5cu, 0x7cu, 0x59u};
  size_t len = strlen(input);
  size_t i;

  if (len != 6) explode_bomb("phase 1 expects exactly 6 characters");
  for (i = 0; i < len; i++) {
    unsigned char transformed = (unsigned char)input[i] ^ key[i];
    if (transformed != target[i]) explode_bomb("phase 1 mismatch");
  }
}

static void phase_2(const char *input) {
  int v[6];
  int i;

  if (sscanf(input, "%d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) != 6) {
    explode_bomb("phase 2 expects six integers");
  }
  if (v[0] != 1 || v[1] != 2) explode_bomb("phase 2 bad prefix");
  for (i = 2; i < 6; i++) {
    if (v[i] != v[i - 1] + v[i - 2] + i) explode_bomb("phase 2 recurrence failed");
  }
}

static int phase3_value(int idx) {
  int acc = 0;

  switch (idx) {
    case 0:
      acc = 0x120;
      acc += 0x15;
      acc -= 0x04;
      break;
    case 1:
      acc = 0x1a0;
      acc -= 0x21;
      acc += 0x05;
      break;
    case 2:
      acc = 0x2b0;
      acc -= 0x44;
      acc -= 0x02;
      break;
    case 3:
      acc = 0x180;
      acc += 0x3a;
      acc -= 0x10;
      break;
    case 4:
      acc = 0x240;
      acc -= 0x17;
      acc += 0x09;
      break;
    case 5:
      acc = 0x155;
      acc += 0x20;
      acc -= 0x0b;
      break;
    case 6:
      acc = 0x2a0;
      acc -= 0x33;
      acc += 0x02;
      break;
    case 7:
      acc = 0x1c0;
      acc += 0x18;
      acc -= 0x01;
      break;
    default:
      explode_bomb("phase 3 index out of range");
  }
  return acc;
}

static void phase_3(const char *input) {
  int idx;
  int value;

  if (sscanf(input, "%d %d", &idx, &value) != 2) explode_bomb("phase 3 expects two integers");
  if (idx < 0 || idx > 7) explode_bomb("phase 3 index out of range");
  if (value != phase3_value(idx)) explode_bomb("phase 3 wrong value");
}

static int func4(int target, int low, int high) {
  int mid;

  if (low > high) return -1;
  mid = low + (high - low) / 2;
  if (target < mid) return mid + func4(target, low, mid - 1);
  if (target > mid) return mid + func4(target, mid + 1, high);
  return mid;
}

static void phase_4(const char *input) {
  int target;
  int expected_sum;
  int actual_sum;

  if (sscanf(input, "%d %d", &target, &expected_sum) != 2) {
    explode_bomb("phase 4 expects two integers");
  }
  if (target < 0 || target > 14) explode_bomb("phase 4 target out of range");

  actual_sum = func4(target, 0, 14);
  if (actual_sum != 37 || expected_sum != 37) explode_bomb("phase 4 wrong path sum");
}

int main(int argc, char **argv) {
  FILE *fp = stdin;
  char line[MAX_LINE];

  if (argc == 2) {
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      perror(argv[1]);
      return 1;
    }
  } else if (argc > 2) {
    fprintf(stderr, "usage: %s [input-file]\n", argv[0]);
    return 2;
  }

  puts("Mini Bomb: 4 phases. One line per phase.");

  read_nonempty_line(fp, line, sizeof(line));
  phase_1(line);
  phase_defused(1);

  read_nonempty_line(fp, line, sizeof(line));
  phase_2(line);
  phase_defused(2);

  read_nonempty_line(fp, line, sizeof(line));
  phase_3(line);
  phase_defused(3);

  read_nonempty_line(fp, line, sizeof(line));
  phase_4(line);
  phase_defused(4);

  puts("Bomb defused. Nice work.");

  if (fp != stdin) fclose(fp);
  return 0;
}
