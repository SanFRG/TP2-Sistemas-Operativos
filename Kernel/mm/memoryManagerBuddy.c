#include <memoryManager.h>

#define BUDDY_MIN_ORDER 5
#define BUDDY_MAX_ORDER 32
#define BUDDY_MAGIC 0xB0DDU

typedef struct buddy_block {
    uint8_t order;
    uint8_t free;
    uint16_t magic;
    uint32_t reserved;
    uint64_t requested_size;
    struct buddy_block *next;
} buddy_block_t;

static uint8_t *heap_base = 0;
static uint64_t heap_capacity = 0;
static uint8_t min_order = BUDDY_MIN_ORDER;
static uint8_t max_order = BUDDY_MIN_ORDER;
static uint64_t header_size_aligned = 0;
static buddy_block_t *free_lists[BUDDY_MAX_ORDER + 1];

static uint64_t used_bytes = 0;
static uint64_t successful_allocations = 0;
static uint64_t successful_frees = 0;
static uint64_t failed_allocations = 0;

static uint64_t align8(uint64_t size) { //TODO:hay funciones que comparte con el memorymanager. podriamos ver de no duplicarlos
    return (size + 7ULL) & ~7ULL;
}

static uint64_t align8_down(uint64_t size) {
    return size & ~7ULL;
}

static uint64_t align8_up_address(uint64_t address) {
    return (address + 7ULL) & ~7ULL;
}

static uint64_t order_size(uint8_t order) {
    return 1ULL << order;
}

static uint8_t largest_order_for(uint64_t size) {
    uint8_t order = 0;

    while ((order + 1) <= BUDDY_MAX_ORDER && (1ULL << (order + 1)) <= size) {
        order++;
    }
    return order;
}

static uint8_t smallest_order_for(uint64_t size) {
    uint8_t order = min_order;

    while (order < max_order && order_size(order) < size) {
        order++;
    }
    return order;
}

static void clear_free_lists(void) {
    for (uint8_t i = 0; i <= BUDDY_MAX_ORDER; i++) {
        free_lists[i] = 0;
    }
}

static void push_free_block(buddy_block_t *block, uint8_t order) {
    block->order = order;
    block->free = 1;
    block->magic = BUDDY_MAGIC;
    block->requested_size = 0;
    block->next = free_lists[order];
    free_lists[order] = block;
}

static buddy_block_t *pop_free_block(uint8_t order) {
    buddy_block_t *block = free_lists[order];

    if (block != 0) {
        free_lists[order] = block->next;
        block->next = 0;
    }
    return block;
}

static uint8_t remove_free_block(buddy_block_t *target, uint8_t order) {
    buddy_block_t *current = free_lists[order];
    buddy_block_t *previous = 0;

    while (current != 0) {
        if (current == target) {
            if (previous == 0) {
                free_lists[order] = current->next;
            } else {
                previous->next = current->next;
            }
            current->next = 0;
            return 1;
        }
        previous = current;
        current = current->next;
    }
    return 0;
}

static uint8_t *block_payload(buddy_block_t *block) {
    return ((uint8_t *)block) + header_size_aligned;
}

static buddy_block_t *payload_block(void *ptr) {
    return (buddy_block_t *)(((uint8_t *)ptr) - header_size_aligned);
}

void mm_init(void *heap_start, uint64_t heap_size) {
    uint64_t aligned_start;
    uint64_t alignment_loss;

    header_size_aligned = align8(sizeof(buddy_block_t));
    min_order = BUDDY_MIN_ORDER;
    while (order_size(min_order) < header_size_aligned + 8ULL) {
        min_order++;
    }

    clear_free_lists();
    used_bytes = 0;
    successful_allocations = 0;
    successful_frees = 0;
    failed_allocations = 0;
    heap_base = 0;
    heap_capacity = 0;
    max_order = min_order;

    if (heap_start == 0 || heap_size <= header_size_aligned) {
        return;
    }

    aligned_start = align8_up_address((uint64_t)heap_start);
    alignment_loss = aligned_start - (uint64_t)heap_start;
    if (heap_size <= alignment_loss) {
        return;
    }

    heap_base = (uint8_t *)aligned_start;
    heap_capacity = align8_down(heap_size - alignment_loss);
    max_order = largest_order_for(heap_capacity);

    if (max_order < min_order) {
        heap_base = 0;
        heap_capacity = 0;
        max_order = min_order;
        return;
    }

    heap_capacity = order_size(max_order);
    push_free_block((buddy_block_t *)heap_base, max_order);
}

void *mm_alloc(uint64_t size) {
    uint64_t requested_size;
    uint64_t required_size;
    uint8_t target_order;
    uint8_t current_order;
    buddy_block_t *block;

    if (heap_base == 0 || size == 0) {
        failed_allocations++;
        return 0;
    }

    requested_size = align8(size);
    required_size = requested_size + header_size_aligned;
    if (required_size > order_size(max_order)) {
        failed_allocations++;
        return 0;
    }

    target_order = smallest_order_for(required_size);
    current_order = target_order;
    while (current_order <= max_order && free_lists[current_order] == 0) {
        current_order++;
    }

    if (current_order > max_order) {
        failed_allocations++;
        return 0;
    }

    block = pop_free_block(current_order);
    while (current_order > target_order) {
        buddy_block_t *buddy;

        current_order--;
        buddy = (buddy_block_t *)(((uint8_t *)block) + order_size(current_order));
        push_free_block(buddy, current_order);
        block->order = current_order;
        block->magic = BUDDY_MAGIC;
    }

    block->order = target_order;
    block->free = 0;
    block->magic = BUDDY_MAGIC;
    block->requested_size = requested_size;
    block->next = 0;

    used_bytes += order_size(target_order);
    successful_allocations++;
    return (void *)block_payload(block);
}

void mm_free(void *ptr) {
    buddy_block_t *block;
    uint8_t order;

    if (ptr == 0 || heap_base == 0) {
        return;
    }

    if ((uint8_t *)ptr < heap_base + header_size_aligned ||
        (uint8_t *)ptr >= heap_base + heap_capacity) {
        return;
    }

    block = payload_block(ptr);
    if ((uint8_t *)block < heap_base ||
        (uint8_t *)block >= heap_base + heap_capacity ||
        block->magic != BUDDY_MAGIC ||
        block->free ||
        block->order < min_order ||
        block->order > max_order) {
        return;
    }

    order = block->order;
    block->free = 1;
    block->requested_size = 0;
    if (used_bytes >= order_size(order)) {
        used_bytes -= order_size(order);
    } else {
        used_bytes = 0;
    }
    successful_frees++;

    while (order < max_order) {
        uint64_t offset = (uint64_t)((uint8_t *)block - heap_base);
        uint64_t buddy_offset = offset ^ order_size(order);
        buddy_block_t *buddy;

        if (buddy_offset >= heap_capacity) {
            break;
        }

        buddy = (buddy_block_t *)(heap_base + buddy_offset);
        if (buddy->magic != BUDDY_MAGIC ||
            !buddy->free ||
            buddy->order != order ||
            !remove_free_block(buddy, order)) {
            break;
        }

        if ((uint8_t *)buddy < (uint8_t *)block) {
            block = buddy;
        }
        order++;
        block->order = order;
        block->free = 1;
        block->magic = BUDDY_MAGIC;
        block->requested_size = 0;
    }

    push_free_block(block, order);
}

void mm_get_status(mm_status_t *out) {
    uint64_t free_bytes = 0;

    if (out == 0) {
        return;
    }

    for (uint8_t order = min_order; order <= max_order; order++) {
        buddy_block_t *current = free_lists[order];
        while (current != 0) {
            free_bytes += order_size(order);
            current = current->next;
        }
    }

    out->total_bytes = heap_capacity;
    out->used_bytes = used_bytes;
    out->free_bytes = free_bytes;
    out->successful_allocations = successful_allocations;
    out->successful_frees = successful_frees;
    out->failed_allocations = failed_allocations;
}
