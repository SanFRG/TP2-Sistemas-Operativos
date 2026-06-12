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

static int remove_waiter_from_sem(semaphore_t *sem, int pid);

// Elige a quien despertar con weighted fair queueing por tiempo virtual:
// saca de la cola al esperante con MENOR vtime (a igual vtime, el que llego
// primero -> FIFO) y le cobra el avance de vtime segun su peso. Como el avance
// es 9/peso, los de mayor prioridad acumulan vtime mas lento y por ende son
// elegidos mas seguido, pero de forma PROPORCIONAL: con pesos {3,3} se turnan
// (ABAB), con {9,3} el de peso 9 sale ~3 de cada 4 veces (ABBB) sin que el
// otro sufra starvation. Esto es lo que hace que subir la prioridad de un
// writer en mvar lo haga aparecer mas, sin monopolizar la salida.
static int dequeue_best_waiter(semaphore_t *sem) {
    int best_pid;
    uint64_t best_vtime;

    if (sem->wait_count <= 0) {
        return -1;
    }

    best_pid = sem->waiters[sem->wait_head];
    best_vtime = process_get_vtime(best_pid);
    for (int i = 1; i < sem->wait_count; i++) {
        int pos = (sem->wait_head + i) % MAX_WAITERS;
        int pid = sem->waiters[pos];
        uint64_t vtime = process_get_vtime(pid);
        if (vtime < best_vtime) {   // estricto: a igual vtime gana el de mas atras (FIFO)
            best_vtime = vtime;
            best_pid = pid;
        }
    }

    process_charge_vtime(best_pid);   // avanza su vtime segun el peso/prioridad

    // remove_waiter_from_sem compacta la cola y saca a best_pid (aparece una
    // sola vez). Reusa la logica ya probada en lugar de mover indices a mano.
    remove_waiter_from_sem(sem, best_pid);
    return best_pid;
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
    int woken = 0;   // 0 = recien llegue; 1 = me desperto un sem_post (ya fui elegido)

    if (!valid_name(name)) {
        return -1;
    }

    my_pid = process_get_current_pid();
    if (my_pid <= 0) {
        return -1;
    }

    while (1) {
        if (process_current_kill_pending()) {
            return -1;
        }

        _cli();
        idx = find_semaphore(name);
        if (idx < 0) {
            _sti();
            return -1;
        }

        sem = &sem_table[idx];

        /* Regla ANTI-BARGING (semaforo justo): un recien llegado (woken==0) solo
         * se queda con el token si NO hay nadie esperando en la cola. Si hay
         * esperantes, NO se cuela: se encola y respeta el orden justo (vtime/FIFO
         * que aplica dequeue_best_waiter). Sin esta regla, el proceso de menor PID
         * -que el scheduler corre primero cada tick- ganaba siempre la carrera del
         * fast-path y le robaba el token al que sem_post acababa de despertar desde
         * la cola, monopolizando la salida (en mvar, un lector imprimia ~4x mas).
         * En cambio, si a MI me desperto un sem_post (woken==1), el token es mio: lo
         * tomo aunque queden otros en cola, porque ya fui el elegido. */
        if (sem->value > 0 && (woken || sem->wait_count == 0)) {
            sem->value--;
            process_sem_held_push(my_pid, idx);  // registra el token para devolverlo si muere
            _sti();
            return 0;
        }

        if (enqueue_waiter(sem, my_pid) != 0) {
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

        /* Despertamos: sem_post nos eligio (dequeue_best_waiter) y nos cobro el
         * vtime. Marcamos woken=1 para tener prioridad sobre los recien llegados al
         * reintentar tomar el token. Nos sacamos de la cola por las dudas (tambien
         * podrian habernos despertado con block/unblock directo). El token vive en
         * value, asi que si nos mataron recien despiertos no se pierde: lo toma el
         * proximo. */
        woken = 1;
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
    process_sem_held_release(process_get_current_pid(), idx);  // ya no debo este token
    sem->value++;   /* el token queda en value, no se entrega "en mano" */

    /* Despierta a un esperante para que reintente. Saltea pids muertos (no
     * deberian quedar en la cola porque process_kill los limpia, pero por las
     * dudas). value queda incrementado: si nadie lo toma ahora, lo tomara el
     * proximo sem_wait. */
    while (sem->wait_count > 0) {
        int pid = dequeue_best_waiter(sem);
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

// Postea un semaforo por indice, sin tocar el tracking de tokens (lo usa
// process.c al morir un proceso para devolver lo que tenia tomado). Misma
// logica de despertar esperantes que sem_post.
int sem_force_post_index(int sem_idx) {
    _cli();
    if (sem_idx < 0 || sem_idx >= MAX_SEMAPHORES || !sem_table[sem_idx].in_use) {
        _sti();
        return -1;
    }

    semaphore_t *sem = &sem_table[sem_idx];
    sem->value++;

    while (sem->wait_count > 0) {
        int pid = dequeue_best_waiter(sem);
        _sti();
        if (process_unblock(pid) == 0) {
            return 0;
        }
        _cli();
        if (!sem_table[sem_idx].in_use) {
            _sti();
            return -1;
        }
        sem = &sem_table[sem_idx];
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
