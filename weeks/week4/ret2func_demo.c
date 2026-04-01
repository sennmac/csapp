#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { BUF_SIZE = 32, MAX_READ = 96 };

typedef void (*check_fn)(uint64_t);

static const uint64_t COOKIE = 0x59b997faULL;

struct Frame {
  char buf[BUF_SIZE];
  check_fn next;
  uint64_t arg;
};

__attribute__((noinline)) static void denied(uint64_t ignored) {
  (void)ignored;
  puts("denied: control flow stayed on the default path");
}

__attribute__((noinline)) static void touch1(uint64_t ignored) {
  (void)ignored;
  puts("touch1: control-flow hijack succeeded");
  exit(0);
}

__attribute__((noinline)) static void touch2(uint64_t value) {
  if (value == COOKIE) {
    printf("touch2: correct cookie 0x%016" PRIx64 "\n", value);
    exit(0);
  }

  printf("touch2: wrong cookie 0x%016" PRIx64 "\n", value);
  exit(1);
}

__attribute__((noinline)) static void run_challenge(void) {
  struct Frame frame;
  ssize_t nread;

  memset(&frame, 0, sizeof(frame));
  frame.next = denied;
  frame.arg = 0;

  printf("buf offset  : %zu\n", offsetof(struct Frame, buf));
  printf("next offset : %zu\n", offsetof(struct Frame, next));
  printf("arg offset  : %zu\n", offsetof(struct Frame, arg));
  printf("touch1      : %p\n", (void *)touch1);
  printf("touch2      : %p\n", (void *)touch2);
  printf("denied      : %p\n", (void *)denied);
  printf("cookie      : 0x%016" PRIx64 "\n", COOKIE);
  puts("goal: overflow buf so that next points to touch1 or touch2");
  puts("challenge: to reach touch2, you must also overwrite arg");

  nread = read(STDIN_FILENO, frame.buf, MAX_READ);
  if (nread < 0) {
    perror("read");
    exit(1);
  }

  printf("read        : %zd bytes\n", nread);
  frame.next(frame.arg);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  run_challenge();
  return 0;
}
