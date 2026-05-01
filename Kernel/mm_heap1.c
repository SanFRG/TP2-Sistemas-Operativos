#include <mm.h>

static uint8_t *heap_base = 0;
static uint64_t heap_capacity = 0;
static uint64_t heap_offset = 0;
static uint64_t successful_allocations = 0;
static uint64_t successful_frees = 0;
static uint64_t failed_allocations = 0;

static uint64_t align8(uint64_t size) {
    return (size + 7ULL) & ~7ULL;
}

void mm_init(void *heap_start, uint64_t heap_size) {
    heap_base = (uint8_t *)heap_start;
    heap_capacity = heap_size;
    heap_offset = 0;
    successful_allocations = 0;
    successful_frees = 0;
    failed_allocations = 0;
}

void *mm_alloc(uint64_t size) {
    uint64_t aligned_size;
    void *result;

    if (heap_base == 0 || size == 0) {
        failed_allocations++;
        return 0;
    }

    aligned_size = align8(size);
    if (aligned_size > (heap_capacity - heap_offset)) {
        failed_allocations++;
        return 0;
    }

    result = heap_base + heap_offset;
    heap_offset += aligned_size;
    successful_allocations++;
    return result;
}

void mm_free(void *ptr) {
    (void)ptr;
    /* heap_1 intentionally does not release memory; count calls for diagnostics */
    successful_frees++;
}

void mm_get_status(mm_status_t *out) {
    if (out == 0) {
        return;
    }

    out->total_bytes = heap_capacity;
    out->used_bytes = heap_offset;
    out->free_bytes = heap_capacity - heap_offset;
    out->successful_allocations = successful_allocations;
    out->successful_frees = successful_frees;
    out->failed_allocations = failed_allocations;
}
