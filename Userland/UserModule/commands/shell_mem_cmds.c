#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <stdint.h>

#define MAX_MM_BLOCKS 128
#define TEST_MM_ITERS 40
#define TEST_MM_MAX_MEMORY (128 * 1024)

typedef struct {
    void *address;
    uint32_t size;
} mm_rq_t;

static uint32_t mm_rand_z = 362436069U;
static uint32_t mm_rand_w = 521288629U;

static uint32_t mm_rand_u32(void) {
    mm_rand_z = 36969U * (mm_rand_z & 65535U) + (mm_rand_z >> 16);
    mm_rand_w = 18000U * (mm_rand_w & 65535U) + (mm_rand_w >> 16);
    return (mm_rand_z << 16) + mm_rand_w;
}

static uint32_t mm_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    return (uint32_t)((mm_rand_u32() + 1.0) * 2.328306435454494e-10 * max);
}

static void mm_fill(void *start, uint8_t value, uint32_t size) {
    uint8_t *p = (uint8_t *)start;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = value;
    }
}

static uint8_t mm_check(const void *start, uint8_t value, uint32_t size) {
    const uint8_t *p = (const uint8_t *)start;
    for (uint32_t i = 0; i < size; i++) {
        if (p[i] != value) {
            return 0;
        }
    }
    return 1;
}

void cmd_mem(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    MemoryStatus status;
    if (mem_status(&status) != 0) {
        println("Error: no se pudo obtener estado de memoria.");
        return;
    }

    println("=== Estado de memoria ===");
    print("Total bytes: ");
    printHex(status.total_bytes);
    print("\n");
    print("Used bytes:  ");
    printHex(status.used_bytes);
    print("\n");
    print("Free bytes:  ");
    printHex(status.free_bytes);
    print("\n");
    print("Allocs ok:   ");
    printHex(status.successful_allocations);
    print("\n");
    print("Frees ok:    ");
    printHex(status.successful_frees);
    print("\n");
    print("Allocs fail: ");
    printHex(status.failed_allocations);
    print("\n");
}

void cmd_memtest(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    MemoryStatus before;
    MemoryStatus after_alloc;
    MemoryStatus after_free;
    void *p1;
    void *p2;

    if (mem_status(&before) != 0) {
        println("memtest: no se pudo leer estado inicial.");
        return;
    }

    p1 = mem_alloc(64);
    p2 = mem_alloc(256);

    if (mem_status(&after_alloc) != 0) {
        println("memtest: no se pudo leer estado despues de alloc.");
        return;
    }

    if (p1 != 0) {
        mem_free(p1);
    }
    if (p2 != 0) {
        mem_free(p2);
    }

    if (mem_status(&after_free) != 0) {
        println("memtest: no se pudo leer estado final.");
        return;
    }

    println("=== memtest ===");
    print("p1(64)  = ");
    printHex((uint64_t)p1);
    print("\n");
    print("p2(256) = ");
    printHex((uint64_t)p2);
    print("\n");

    println("--- before ---");
    print("used: ");
    printHex(before.used_bytes);
    print(" free: ");
    printHex(before.free_bytes);
    print(" allocs: ");
    printHex(before.successful_allocations);
    print(" frees: ");
    printHex(before.successful_frees);
    print(" fails: ");
    printHex(before.failed_allocations);
    print("\n");

    println("--- after alloc ---");
    print("used: ");
    printHex(after_alloc.used_bytes);
    print(" free: ");
    printHex(after_alloc.free_bytes);
    print(" allocs: ");
    printHex(after_alloc.successful_allocations);
    print(" frees: ");
    printHex(after_alloc.successful_frees);
    print(" fails: ");
    printHex(after_alloc.failed_allocations);
    print("\n");

    println("--- after free ---");
    print("used: ");
    printHex(after_free.used_bytes);
    print(" free: ");
    printHex(after_free.free_bytes);
    print(" allocs: ");
    printHex(after_free.successful_allocations);
    print(" frees: ");
    printHex(after_free.successful_frees);
    print(" fails: ");
    printHex(after_free.failed_allocations);
    print("\n");
}

void cmd_test_mm(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    mm_rq_t reqs[MAX_MM_BLOCKS];
    uint32_t total;
    uint8_t rq;
    uint32_t i;

    println("test_mm: iniciando (iteraciones acotadas)...");

    for (int iter = 0; iter < TEST_MM_ITERS; iter++) {
        rq = 0;
        total = 0;

        while (rq < MAX_MM_BLOCKS && total < TEST_MM_MAX_MEMORY) {
            uint32_t remaining = TEST_MM_MAX_MEMORY - total;
            uint32_t size = mm_uniform(remaining);
            if (size == 0) {
                size = 1;
            }

            reqs[rq].size = size;
            reqs[rq].address = mem_alloc(size);
            if (reqs[rq].address != 0) {
                total += size;
                rq++;
            } else {
                break;
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0) {
                mm_fill(reqs[i].address, (uint8_t)i, reqs[i].size);
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0 && !mm_check(reqs[i].address, (uint8_t)i, reqs[i].size)) {
                println("test_mm ERROR: corrupcion detectada");
                return;
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0) {
                mem_free(reqs[i].address);
            }
        }
    }

    println("test_mm OK");
}
