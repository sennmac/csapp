#ifndef WEEK6_ALLOCATOR_H
#define WEEK6_ALLOCATOR_H

#include <stddef.h>

int lab_init(size_t arena_size);
void *lab_malloc(size_t size);
void lab_free(void *ptr);
void lab_dump(void);
size_t lab_bytes_used(void);
size_t lab_bytes_capacity(void);

#endif
