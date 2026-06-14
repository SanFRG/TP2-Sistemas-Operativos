#include <shell.h>
#include <lib.h>
#include <stdint.h>





#define MAX_TEST_PROC 16

typedef enum { TP_RUNNING, TP_BLOCKED, TP_KILLED } tp_state;

typedef struct {
    int64_t pid;
    tp_state state;
} tp_rq;

static uint32_t tp_rand_z = 192837465U;
static uint32_t tp_rand_w = 564738291U;

static uint32_t tp_rand_u32(void) {
    tp_rand_z = 36969U * (tp_rand_z & 65535U) + (tp_rand_z >> 16);
    tp_rand_w = 18000U * (tp_rand_w & 65535U) + (tp_rand_w >> 16);
    return (tp_rand_z << 16) + tp_rand_w;
}



static uint32_t tp_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    return (uint32_t)(((uint64_t)tp_rand_u32() * max) >> 32);
}


static void tp_endless_loop(void *arg) {
    (void)arg;
    while (1) {
        yield_cpu();
    }
}

void cmd_testproc(int argc, char *argv[]) {
    tp_rq rqs[MAX_TEST_PROC];
    int max_processes;
    int alive;

    if (argc != 2) {
        println("Uso: testproc <cantidad_de_procesos>");
        return;
    }

    max_processes = atoi(argv[1]);
    if (max_processes <= 0 || max_processes > MAX_TEST_PROC) {
        print("testproc: cantidad debe estar entre 1 y ");
        printInt(MAX_TEST_PROC);
        println(".");
        return;
    }



    while (1) {
        alive = 0;


        for (int rq = 0; rq < max_processes; rq++) {
            rqs[rq].pid = create_process("endless_loop", tp_endless_loop, 0, 1, 0);
            if (rqs[rq].pid < 0) {
                println("testproc: ERROR creando proceso");

                for (int j = 0; j < rq; j++) {
                    if (rqs[j].state != TP_KILLED) {
                        kill_process(rqs[j].pid);
                        waitpid(rqs[j].pid);
                    }
                }
                return;
            }
            rqs[rq].state = TP_RUNNING;
            alive++;
        }


        while (alive > 0) {
            for (int rq = 0; rq < max_processes; rq++) {
                switch (tp_uniform(2)) {
                    case 0:
                        if (rqs[rq].state == TP_RUNNING ||
                            rqs[rq].state == TP_BLOCKED) {
                            if (kill_process(rqs[rq].pid) != 0) {
                                println("testproc: ERROR matando proceso");
                                return;
                            }
                            rqs[rq].state = TP_KILLED;
                            waitpid(rqs[rq].pid);
                            alive--;
                        }
                        break;
                    case 1:
                        if (rqs[rq].state == TP_RUNNING) {
                            if (block_process(rqs[rq].pid) != 0) {
                                println("testproc: ERROR bloqueando proceso");
                                return;
                            }
                            rqs[rq].state = TP_BLOCKED;
                        }
                        break;
                }
            }


            for (int rq = 0; rq < max_processes; rq++) {
                if (rqs[rq].state == TP_BLOCKED && (tp_uniform(2) == 1)) {
                    if (unblock_process(rqs[rq].pid) != 0) {
                        println("testproc: ERROR desbloqueando proceso");
                        return;
                    }
                    rqs[rq].state = TP_RUNNING;
                }
            }
        }
    }
}
