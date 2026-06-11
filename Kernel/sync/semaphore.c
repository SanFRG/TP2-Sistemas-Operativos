#include <semaphore.h>
#include <interrupts.h>
#include <process.h>
#include <lib.h>
#include <stdint.h>

#define MAX_SEMAPHORES 32
#define SEM_NAME_LEN 32
#define MAX_WAITERS MAX_PROCESSES

typedef struct { 
    uint8_t in_use; //indica si el semaforo esta en uso o no, para saber si se puede usar ese slot o no
    char name[SEM_NAME_LEN];
    int64_t value; //valor que cuenta lo de post y wait (negativos, cero o uno)
    uint64_t ref_count; //cuenta cuantas veces se ha abierto el semaforo, para saber cuando eliminarlo
    int waiters[MAX_WAITERS];
    int wait_head;
    int wait_tail;
    int wait_count;
} semaphore_t;

static semaphore_t sem_table[MAX_SEMAPHORES];

static int str_eq(const char *a, const char *b) {
    int i = 0;
    if (a == 0 || b == 0) {
        return 0;
    }
    while (a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == b[i];
}

static int valid_name(const char *name) {
    int i = 0;
    if (name == 0 || name[0] == '\0') {
        return 0;
    }
    while (name[i] != 0) {
        if (i >= SEM_NAME_LEN - 1) {
            return 0;
        }
        i++;
    }
    return 1;
}

static int find_semaphore(const char *name) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].in_use && str_eq(sem_table[i].name, name)) {
            return i;
        }
    }
    return -1;
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (!sem_table[i].in_use) {
            return i;
        }
    }
    return -1;
}

static int enqueue_waiter(semaphore_t *sem, int pid) {
    if (sem->wait_count >= MAX_WAITERS) {
        return -1;
    }
    sem->waiters[sem->wait_tail] = pid;
    sem->wait_tail = (sem->wait_tail + 1) % MAX_WAITERS;
    sem->wait_count++;
    return 0;
}

static int dequeue_waiter(semaphore_t *sem) {
    int pid;
    if (sem->wait_count <= 0) {
        return -1;
    }
    pid = sem->waiters[sem->wait_head];
    sem->wait_head = (sem->wait_head + 1) % MAX_WAITERS;
    sem->wait_count--;
    return pid;
}

static int remove_waiter_from_sem(semaphore_t *sem, int pid) {
    int kept[MAX_WAITERS];
    int kept_count = 0;
    int removed = 0;
    int original_count;

    if (sem == 0 || pid <= 0 || sem->wait_count <= 0) {
        return 0;
    }

    original_count = sem->wait_count;
    for (int i = 0; i < original_count; i++) {
        int current = sem->waiters[(sem->wait_head + i) % MAX_WAITERS];
        if (current == pid) {
            removed++;
        } else {
            kept[kept_count++] = current;
        }
    }

    for (int i = 0; i < kept_count; i++) {
        sem->waiters[i] = kept[i];
    }
    sem->wait_head = 0;
    sem->wait_tail = kept_count % MAX_WAITERS;
    sem->wait_count = kept_count;
    return removed;
}

void sem_system_init(void) {
    memset(sem_table, 0, sizeof(sem_table));
}

int sem_open(const char *name, uint64_t initial_value) {
    int idx;

    if (!valid_name(name)) {
        return -1;
    }

    _cli();
    idx = find_semaphore(name);
    if (idx >= 0) {
        sem_table[idx].ref_count++;
        _sti();
        return 0;
    }

    idx = find_free_slot();
    if (idx < 0) {
        _sti();
        return -1;
    }

    sem_table[idx].in_use = 1;
    strncpy(sem_table[idx].name, name, SEM_NAME_LEN);
    sem_table[idx].value = (int64_t)initial_value;
    sem_table[idx].ref_count = 1;
    sem_table[idx].wait_head = 0;
    sem_table[idx].wait_tail = 0;
    sem_table[idx].wait_count = 0;
    _sti();
    return 0;
}

int sem_close(const char *name) {
    int idx;
    semaphore_t *sem;

    if (!valid_name(name)) {
        return -1;
    }

    _cli();
    idx = find_semaphore(name);
    if (idx < 0) {
        _sti();
        return -1;
    }

    sem = &sem_table[idx];
    if (sem->ref_count == 0) {
        _sti();
        return -1;
    }

    sem->ref_count--;

    if (sem->ref_count == 0 && sem->wait_count == 0) {
        memset(sem, 0, sizeof(*sem));
    }

    _sti();
    return 0;
}

/* Semaforo basado en value (no en handoff directo): el "token" vive siempre en
 * sem->value. sem_post incrementa value y despierta a un esperante para que
 * REINTENTE tomarlo; el esperante, al despertar, vuelve a chequear value. Asi,
 * si se mata a un proceso recien despertado (READY pero que todavia no corrio),
 * el token no se pierde: value quedo incrementado y lo toma otro. Esto evita el
 * deadlock al matar procesos que usan semaforos (p. ej. mvar). */
int sem_wait(const char *name) {
    int idx;
    semaphore_t *sem;
    int my_pid;

    if (!valid_name(name)) {
        return -1;
    }

    while (1) {
        _cli();
        idx = find_semaphore(name);
        if (idx < 0) {
            _sti();
            return -1;
        }

        sem = &sem_table[idx];
        if (sem->value > 0) {
            sem->value--;
            _sti();
            return 0;
        }

        my_pid = process_get_current_pid();
        if (my_pid <= 0 || enqueue_waiter(sem, my_pid) != 0) {
            _sti();
            return -1;
        }

        if (process_block_current() != 0) {
            remove_waiter_from_sem(sem, my_pid);
            _sti();
            return -1;
        }

        _sti();
        _yield();

        /* Despertamos. Nos sacamos de la cola (sem_post ya nos dequeueo, pero
         * tambien podrian habernos despertado con 'block/unblock') y el bucle
         * reintenta tomar value. Si otro lo gano, value sera 0 y nos volvemos
         * a bloquear: no hay busy-wait porque cada vuelta hace _yield. */
        _cli();
        idx = find_semaphore(name);
        if (idx >= 0) {
            remove_waiter_from_sem(&sem_table[idx], my_pid);
        }
        _sti();
    }
}

int sem_post(const char *name) {
    int idx;
    semaphore_t *sem;

    if (!valid_name(name)) {
        return -1;
    }

    _cli();
    idx = find_semaphore(name);
    if (idx < 0) {
        _sti();
        return -1;
    }

    sem = &sem_table[idx];
    sem->value++;   /* el token queda en value, no se entrega "en mano" */

    /* Despierta a un esperante para que reintente. Saltea pids muertos (no
     * deberian quedar en la cola porque process_kill los limpia, pero por las
     * dudas). value queda incrementado: si nadie lo toma ahora, lo tomara el
     * proximo sem_wait. */
    while (sem->wait_count > 0) {
        int pid = dequeue_waiter(sem);
        _sti();
        if (process_unblock(pid) == 0) {
            return 0;
        }
        _cli();
        idx = find_semaphore(name);
        if (idx < 0 || !sem_table[idx].in_use) {
            _sti();
            return -1;
        }
        sem = &sem_table[idx];
    }

    _sti();
    return 0;
}

void sem_remove_waiter(int pid) {
    if (pid <= 0) {
        return;
    }

    _cli();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].in_use) {
            if (remove_waiter_from_sem(&sem_table[i], pid) > 0) {
                if (sem_table[i].ref_count == 0 && sem_table[i].wait_count == 0) {
                    memset(&sem_table[i], 0, sizeof(sem_table[i]));
                }
            }
        }
    }
    _sti();
}
