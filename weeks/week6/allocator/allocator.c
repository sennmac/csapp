#include "allocator.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { ARENA_CAPACITY = 64 * 1024, ALIGNMENT = 8 };

typedef struct BlockHeader {
  size_t size;
  bool is_free;
  struct BlockHeader *next;
} BlockHeader;

static unsigned char arena[ARENA_CAPACITY];
static size_t arena_limit;
static size_t arena_used;
static BlockHeader *head;
static BlockHeader *tail;

static size_t align_up(size_t value) {
  return (value + (ALIGNMENT - 1)) & ~(size_t)(ALIGNMENT - 1);
}

static size_t header_size(void) {
  return align_up(sizeof(BlockHeader));
}

static size_t block_offset(const BlockHeader *block) {
  return (size_t)((const unsigned char *)block - arena);
}

static void *block_payload(BlockHeader *block) {
  return (void *)((unsigned char *)block + header_size());
}

static BlockHeader *payload_to_block(void *ptr) {
  return (BlockHeader *)((unsigned char *)ptr - header_size());
}

static BlockHeader *find_reusable_block(size_t size) {
  BlockHeader *cur = head;

  /*
   * Day 3 TODO:
   * Replace this stub with a first-fit search over the free list.
   * Suggested rule: reuse the first block where is_free && block->size >= size.
   */
  (void)size;
  (void)cur;
  return NULL;
}

static void maybe_split_block(BlockHeader *block, size_t requested_size) {
  /*
   * Day 4 TODO:
   * If block is much larger than requested_size, split it into:
   *   [allocated block][new free block]
   * Update next pointers and tail if needed.
   */
  (void)block;
  (void)requested_size;
}

static void coalesce_neighbors(void) {
  BlockHeader *cur = head;

  /*
   * Day 4 TODO:
   * Merge adjacent free blocks. Because this teaching allocator keeps
   * blocks in address order, a single forward pass is enough.
   */
  (void)cur;
}

int lab_init(size_t arena_size) {
  if (arena_size == 0 || arena_size > ARENA_CAPACITY) {
    return -1;
  }

  memset(arena, 0, sizeof(arena));
  arena_limit = align_up(arena_size);
  arena_used = 0;
  head = NULL;
  tail = NULL;
  return 0;
}

void *lab_malloc(size_t size) {
  BlockHeader *block;
  size_t payload_size;
  size_t total_size;

  if (size == 0) {
    return NULL;
  }

  payload_size = align_up(size);
  block = find_reusable_block(payload_size);
  if (block != NULL) {
    block->is_free = false;
    maybe_split_block(block, payload_size);
    return block_payload(block);
  }

  total_size = header_size() + payload_size;
  if (arena_used + total_size > arena_limit) {
    return NULL;
  }

  block = (BlockHeader *)(arena + arena_used);
  block->size = payload_size;
  block->is_free = false;
  block->next = NULL;

  if (tail != NULL) {
    tail->next = block;
  } else {
    head = block;
  }
  tail = block;
  arena_used += total_size;

  return block_payload(block);
}

void lab_free(void *ptr) {
  BlockHeader *block;

  if (ptr == NULL) {
    return;
  }

  block = payload_to_block(ptr);
  block->is_free = true;
  coalesce_neighbors();
}

void lab_dump(void) {
  BlockHeader *cur = head;
  size_t index = 0;

  printf("arena used/capacity: %zu / %zu bytes\n", arena_used, arena_limit);
  while (cur != NULL) {
    printf("block[%zu] offset=%zu size=%zu state=%s\n",
           index,
           block_offset(cur),
           cur->size,
           cur->is_free ? "free" : "used");
    cur = cur->next;
    ++index;
  }
}

size_t lab_bytes_used(void) {
  return arena_used;
}

size_t lab_bytes_capacity(void) {
  return arena_limit;
}
