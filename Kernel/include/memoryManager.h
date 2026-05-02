#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint64_t successful_allocations;
    uint64_t successful_frees;
    uint64_t failed_allocations;
} mm_status_t;

void mm_init(void *heap_start, uint64_t heap_size);
void *mm_alloc(uint64_t size);
void mm_free(void *ptr);
void mm_get_status(mm_status_t *out);

#endif
