#include <test_sync_userland.h>
#include <lib.h>
#include <stdint.h>

#define TEST_SYNC_SEM "test_sync_sem"
#define TEST_SYNC_MAX_PAIRS 16

typedef struct {
    uint64_t iterations;
    int delta;
    int use_sem;
} sync_worker_args_t;

static int64_t shared_value = 0;




static uint32_t sync_rand_z = 362436069U;
static uint32_t sync_rand_w = 521288629U;

static uint32_t sync_rand_u32(void) {
    sync_rand_z = 36969U * (sync_rand_z & 65535U) + (sync_rand_z >> 16);
    sync_rand_w = 18000U * (sync_rand_w & 65535U) + (sync_rand_w >> 16);
    return (sync_rand_z << 16) + sync_rand_w;
}


static uint32_t sync_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    return (uint32_t)(((uint64_t)sync_rand_u32() * max) >> 32);
}

static void slow_inc(int64_t *value, int delta) {
    int64_t aux = *value;



    if (sync_uniform(100) < 30) {
        yield_cpu();
    }
    aux += delta;
    *value = aux;
}

static void sync_worker_entry(void *arg) {
    sync_worker_args_t *cfg = (sync_worker_args_t *)arg;
    if (cfg == 0) {
        return;
    }

    if (cfg->use_sem && sem_open(TEST_SYNC_SEM, 1) != 0) {
        return;
    }

    for (uint64_t i = 0; i < cfg->iterations; i++) {
        if (cfg->use_sem && sem_wait(TEST_SYNC_SEM) != 0) {
            break;
        }

        slow_inc(&shared_value, cfg->delta);

        if (cfg->use_sem && sem_post(TEST_SYNC_SEM) != 0) {
            break;
        }
    }

    if (cfg->use_sem) {
        sem_close(TEST_SYNC_SEM);
    }
}





void cmd_test_sync(int argc, char *argv[]) {
    sync_worker_args_t args[TEST_SYNC_MAX_PAIRS * 2];
    int64_t pids[TEST_SYNC_MAX_PAIRS * 2];

    if (argc != 4) {
        println("Uso: test_sync <pares> <iteraciones> <use_sem: 0|1>");
        return;
    }

    int pairs = atoi(argv[1]);
    int parsed_iterations = atoi(argv[2]);
    int use_sem = atoi(argv[3]);

    if (pairs <= 0 || pairs > TEST_SYNC_MAX_PAIRS) {
        print("test_sync: pares debe estar entre 1 y ");
        printInt(TEST_SYNC_MAX_PAIRS);
        println(".");
        return;
    }
    if (parsed_iterations <= 0 || (use_sem != 0 && use_sem != 1)) {
        println("Uso: test_sync <pares> <iteraciones> <use_sem: 0|1>");
        return;
    }
    uint64_t iterations = (uint64_t)parsed_iterations;
    int total = pairs * 2;

    shared_value = 0;

    for (int i = 0; i < total; i++) {
        pids[i] = -1;
    }

    int failed = 0;
    for (int i = 0; i < pairs && !failed; i++) {
        int dec_idx = i;
        int inc_idx = i + pairs;

        args[dec_idx].iterations = iterations;
        args[dec_idx].delta = -1;
        args[dec_idx].use_sem = use_sem;

        args[inc_idx].iterations = iterations;
        args[inc_idx].delta = 1;
        args[inc_idx].use_sem = use_sem;

        pids[dec_idx] = create_process("sync_dec", sync_worker_entry, &args[dec_idx], 1, 0);
        pids[inc_idx] = create_process("sync_inc", sync_worker_entry, &args[inc_idx], 1, 0);

        if (pids[dec_idx] < 0 || pids[inc_idx] < 0) {
            println("test_sync: error creando procesos.");
            failed = 1;
        }
    }


    for (int i = 0; i < total; i++) {
        if (pids[i] >= 0) {
            waitpid(pids[i]);
        }
    }

    if (failed) {
        return;
    }

    print("Final value: ");
    printInt((int)shared_value);
    print("  use_sem=");
    printInt(use_sem);
    println("");
}
