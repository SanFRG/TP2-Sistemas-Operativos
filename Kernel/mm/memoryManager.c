#include <memoryManager.h>
#include <interrupts.h>

typedef struct block_header {
    uint64_t size;
    uint8_t free;
    struct block_header *next;
} block_header_t;

static uint8_t *heap_base = 0;
static uint8_t *heap_limit = 0;
static uint64_t heap_capacity = 0;
static block_header_t *first_block = 0;
static uint64_t used_bytes = 0;
static uint64_t successful_allocations = 0;
static uint64_t successful_frees = 0;
static uint64_t failed_allocations = 0;

static uint64_t header_size_aligned = 0;

static uint64_t align8(uint64_t size) {
    return (size + 7ULL) & ~7ULL;
}

static uint64_t align8_down(uint64_t size) {
    return size & ~7ULL;
}

static uint8_t *block_payload(block_header_t *block) {
    return ((uint8_t *)block) + header_size_aligned;
}

static block_header_t *payload_block(void *ptr) {
    return (block_header_t *)(((uint8_t *)ptr) - header_size_aligned);
}

static void split_block(block_header_t *block, uint64_t requested_size) {
    uint64_t remaining_size;
    block_header_t *new_block;
    uint8_t *new_block_addr;
    uint8_t *new_payload_end;

    if (block == 0) {
        return;
    }

    if (block->size <= requested_size + header_size_aligned + 8ULL) {
        return;
    }

    remaining_size = block->size - requested_size - header_size_aligned;
    new_block_addr = block_payload(block) + requested_size;
    new_payload_end = new_block_addr + header_size_aligned + remaining_size;

    if (new_payload_end > heap_limit) {
        return;
    }

    new_block = (block_header_t *)new_block_addr;
    new_block->size = remaining_size;
    new_block->free = 1;
    new_block->next = block->next;

    block->size = requested_size;
    block->next = new_block;
}

static void coalesce_next(block_header_t *block) {
    block_header_t *next;

    if (block == 0) {
        return;
    }

    next = block->next;
    if (next != 0 && block->free && next->free) {
        block->size += header_size_aligned + next->size;
        block->next = next->next;
    }
}

void mm_init(void *heap_start, uint64_t heap_size) {
    uint64_t aligned_capacity;

    header_size_aligned = align8(sizeof(block_header_t));
    heap_base = (uint8_t *)heap_start;
    /* Never grow heap beyond the reserved region. */
    aligned_capacity = align8_down(heap_size);
    heap_capacity = aligned_capacity;
    heap_limit = heap_base + heap_capacity;
    first_block = 0;
    used_bytes = 0;
    successful_allocations = 0;
    successful_frees = 0;
    failed_allocations = 0;

    if (heap_base == 0 || heap_capacity <= header_size_aligned) {
        heap_capacity = 0;
        heap_limit = heap_base;
        return;
    }

    first_block = (block_header_t *)heap_base;
    first_block->size = heap_capacity - header_size_aligned;
    first_block->free = 1;
    first_block->next = 0;
}

/* Las operaciones recorren y modifican la lista de bloques, que es estado
 * global compartido. Como mm_alloc/mm_free se llaman tanto desde contexto de
 * proceso (syscalls) como desde el scheduler en contexto de interrupcion
 * (clean_orphan libera stacks de huerfanos), se protegen con irqsave/irqrestore
 * para que no se pisen entre si y corrompan la lista. */
void *mm_alloc(uint64_t size) {
    uint64_t requested_size;
    block_header_t *current;
    uint64_t flags;

    if (heap_base == 0 || size == 0) {
        failed_allocations++;
        return 0;
    }

    requested_size = align8(size);

    flags = _save_irq();
    current = first_block;
    while (current != 0) {
        if (current->free && current->size >= requested_size) {
            split_block(current, requested_size);
            current->free = 0;
            used_bytes += current->size;
            successful_allocations++;
            _restore_irq(flags);
            return (void *)block_payload(current);
        }
        current = current->next;
    }

    failed_allocations++;
    _restore_irq(flags);
    return 0;
}

void mm_free(void *ptr) {
    block_header_t *current;
    block_header_t *previous;
    block_header_t *block;
    uint64_t flags;

    if (ptr == 0 || first_block == 0) {
        return;
    }

    if ((uint8_t *)ptr < heap_base + header_size_aligned || (uint8_t *)ptr >= heap_limit) {
        return;
    }

    block = payload_block(ptr);

    flags = _save_irq();
    current = first_block;
    previous = 0;

    while (current != 0 && current != block) {
        previous = current;
        current = current->next;
    }

    if (current == 0 || current->free) {
        _restore_irq(flags);
        return;
    }

    current->free = 1;
    if (used_bytes >= current->size) {
        used_bytes -= current->size;
    } else {
        used_bytes = 0;
    }
    successful_frees++;

    coalesce_next(current);
    if (previous != 0) {
        coalesce_next(previous);
    }
    _restore_irq(flags);
}

void mm_get_status(mm_status_t *out) {
    block_header_t *current;
    uint64_t free_bytes;
    uint64_t flags;

    if (out == 0) {
        return;
    }

    flags = _save_irq();
    free_bytes = 0;
    current = first_block;
    while (current != 0) {
        if (current->free) {
            free_bytes += current->size;
        }
        current = current->next;
    }

    out->total_bytes = heap_capacity;
    out->used_bytes = used_bytes;
    out->free_bytes = free_bytes;
    out->successful_allocations = successful_allocations;
    out->successful_frees = successful_frees;
    out->failed_allocations = failed_allocations;
    _restore_irq(flags);
}
