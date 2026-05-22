#include <test_sync_userland.h>
#include <lib.h>
#include <stdint.h>

#define TEST_SYNC_SEM "test_sync_sem"
#define TEST_SYNC_PAIRS 2

typedef struct {
    uint64_t iterations;
    int delta;
    int use_sem;
} sync_worker_args_t;

static int64_t shared_value = 0;

static void slow_inc(int64_t *value, int delta, uint64_t step) {
    int64_t aux = *value;
    if ((step & 3) == 0) {
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

        slow_inc(&shared_value, cfg->delta, i);

        if (cfg->use_sem && sem_post(TEST_SYNC_SEM) != 0) {
            break;
        }
    }

    if (cfg->use_sem) {
        sem_close(TEST_SYNC_SEM);
    }
}

void cmd_test_sync(int argc, char *argv[]) {
    uint64_t iterations;
    int use_sem;
    sync_worker_args_t args[TEST_SYNC_PAIRS * 2];
    int64_t pids[TEST_SYNC_PAIRS * 2];

    if (argc != 3) {
        println("Uso: test_sync <iteraciones> <use_sem: 0|1>");
        return;
    }

    int parsed_iterations = atoi(argv[1]);
    use_sem = atoi(argv[2]);

    if (parsed_iterations <= 0 || (use_sem != 0 && use_sem != 1)) {
        println("Uso: test_sync <iteraciones> <use_sem: 0|1>");
        return;
    }
    iterations = (uint64_t)parsed_iterations;

    shared_value = 0;

    for (int i = 0; i < TEST_SYNC_PAIRS; i++) {
        int dec_idx = i;
        int inc_idx = i + TEST_SYNC_PAIRS;

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
            return;
        }
    }

    for (int i = 0; i < TEST_SYNC_PAIRS * 2; i++) {
        waitpid(pids[i]);
    }

    print("Final value: ");
    printInt((int)shared_value);
    print("  use_sem=");
    printInt(use_sem);
    println("");
}
