#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { BUF_SIZE = 32, MAX_READ = 256 };

static void dump_bytes(const unsigned char *buf, size_t count) {
  size_t i;

  if (count == 0) {
    puts("buffer preview: <empty>");
    return;
  }

  puts("buffer preview:");
  for (i = 0; i < count; ++i) {
    printf("%02x ", buf[i]);
  }
  puts("");
}

__attribute__((noinline)) static void unsafe_copy(void) {
  char buf[BUF_SIZE];
  volatile uint64_t marker = 0x1122334455667788ULL;
  void *frame = __builtin_frame_address(0);
  ssize_t nread;
  size_t shown = 0;

  memset(buf, 0, sizeof(buf));
  printf("build      : %s\n",
#ifdef PROTECTED_BUILD
         "protected"
#else
         "unprotected"
#endif
  );
  printf("buf        : %p\n", (void *)buf);
  printf("marker     : %p\n", (const void *)&marker);
  printf("frame      : %p\n", frame);
  printf("ret-slot~  : %p\n", (void *)((char *)frame + sizeof(void *)));
  printf("enter up to %d bytes; more than %d bytes will overflow buf\n",
         MAX_READ, BUF_SIZE);

  nread = read(STDIN_FILENO, buf, MAX_READ);
  if (nread < 0) {
    perror("read");
    exit(1);
  }

  if (nread > 0) {
    shown = (size_t)nread < sizeof(buf) ? (size_t)nread : sizeof(buf);
  }

  printf("read       : %zd bytes\n", nread);
  dump_bytes((const unsigned char *)buf, shown);
  printf("marker now : 0x%016" PRIx64 "\n", marker);
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  puts("overflow_demo: inspect the stack layout, then try a longer input.");
  unsafe_copy();
  puts("returned normally");
  return 0;
}
