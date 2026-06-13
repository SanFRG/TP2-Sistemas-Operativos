#include <semaphore.h>
#include <interrupts.h>
#include <process.h>
#include <lib.h>
#include <stdint.h>

#define MAX_SEMAPHORES 32
#define SEM_NAME_LEN 32
#define MAX_WAITERS MAX_PROCESSES

typedef struct {
    uint8_t in_use;
    char name[SEM_NAME_LEN];
    int64_t value;
    uint64_t ref_count;
    int waiters[MAX_WAITERS];
    int wait_head;
    int wait_tail;
    int wait_count;
} semaphore_t;

static semaphore_t sem_table[MAX_SEMAPHORES];
static uint8_t sem_table_lock = 0;

static uint64_t sem_lock_acquire(void) {
    uint64_t flags = _save_irq();
    while (_atomic_xchg_u8(&sem_table_lock, 1) != 0) {
    }
    return flags;
}

static void sem_lock_release(uint64_t flags) {
    sem_table_lock = 0;
    _restore_irq(flags);
}

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
        if (vtime < best_vtime) {
            best_vtime = vtime;
            best_pid = pid;
        }
    }

    process_charge_vtime(best_pid);
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
    uint64_t flags;

    if (!valid_name(name)) {
        return -1;
    }

    flags = sem_lock_acquire();
    idx = find_semaphore(name);
    if (idx >= 0) {
        sem_table[idx].ref_count++;
        sem_lock_release(flags);
        return 0;
    }

    idx = find_free_slot();
    if (idx < 0) {
        sem_lock_release(flags);
        return -1;
    }

    sem_table[idx].in_use = 1;
    strncpy(sem_table[idx].name, name, SEM_NAME_LEN);
    sem_table[idx].value = (int64_t)initial_value;
    sem_table[idx].ref_count = 1;
    sem_table[idx].wait_head = 0;
    sem_table[idx].wait_tail = 0;
    sem_table[idx].wait_count = 0;
    sem_lock_release(flags);
    return 0;
}

int sem_close(const char *name) {
    int idx;
    semaphore_t *sem;
    uint64_t flags;

    if (!valid_name(name)) {
        return -1;
    }

    flags = sem_lock_acquire();
    idx = find_semaphore(name);
    if (idx < 0) {
        sem_lock_release(flags);
        return -1;
    }

    sem = &sem_table[idx];
    if (sem->ref_count == 0) {
        sem_lock_release(flags);
        return -1;
    }

    sem->ref_count--;

    if (sem->ref_count == 0 && sem->wait_count == 0) {
        memset(sem, 0, sizeof(*sem));
    }

    sem_lock_release(flags);
    return 0;
}

int sem_wait(const char *name) {
    int idx;
    semaphore_t *sem;
    int my_pid;
    int woken = 0;
    uint64_t flags;

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

        flags = sem_lock_acquire();
        idx = find_semaphore(name);
        if (idx < 0) {
            sem_lock_release(flags);
            return -1;
        }

        sem = &sem_table[idx];

        if (sem->value > 0 && (woken || sem->wait_count == 0)) {
            sem->value--;
            process_sem_held_push(my_pid, idx);
            sem_lock_release(flags);
            return 0;
        }

        if (enqueue_waiter(sem, my_pid) != 0) {
            sem_lock_release(flags);
            return -1;
        }

        if (process_block_current() != 0) {
            remove_waiter_from_sem(sem, my_pid);
            sem_lock_release(flags);
            return -1;
        }

        sem_lock_release(flags);
        _yield();

        woken = 1;
        flags = sem_lock_acquire();
        idx = find_semaphore(name);
        if (idx >= 0) {
            remove_waiter_from_sem(&sem_table[idx], my_pid);
        }
        sem_lock_release(flags);
    }
}

static int sem_wake_waiters(int idx, uint64_t flags) {
    semaphore_t *sem = &sem_table[idx];
    while (sem->wait_count > 0) {
        int pid = dequeue_best_waiter(sem);
        sem_lock_release(flags);
        if (process_unblock(pid) == 0) {
            return 0;
        }
        flags = sem_lock_acquire();
        if (!sem_table[idx].in_use) {
            sem_lock_release(flags);
            return -1;
        }
        sem = &sem_table[idx];
    }

    sem_lock_release(flags);
    return 0;
}

int sem_post(const char *name) {
    int idx;
    uint64_t flags;

    if (!valid_name(name)) {
        return -1;
    }

    flags = sem_lock_acquire();
    idx = find_semaphore(name);
    if (idx < 0) {
        sem_lock_release(flags);
        return -1;
    }

    process_sem_held_release(process_get_current_pid(), idx);
    sem_table[idx].value++;

    return sem_wake_waiters(idx, flags);
}

int sem_force_post_index(int sem_idx) {
    uint64_t flags = sem_lock_acquire();
    if (sem_idx < 0 || sem_idx >= MAX_SEMAPHORES || !sem_table[sem_idx].in_use) {
        sem_lock_release(flags);
        return -1;
    }

    sem_table[sem_idx].value++;
    return sem_wake_waiters(sem_idx, flags);
}

void sem_remove_waiter(int pid) {
    uint64_t flags;

    if (pid <= 0) {
        return;
    }

    flags = sem_lock_acquire();
    for (int i = 0; i < MAX_SEMAPHORES; i++) {
        if (sem_table[i].in_use) {
            if (remove_waiter_from_sem(&sem_table[i], pid) > 0) {
                if (sem_table[i].ref_count == 0 && sem_table[i].wait_count == 0) {
                    memset(&sem_table[i], 0, sizeof(sem_table[i]));
                }
            }
        }
    }
    sem_lock_release(flags);
}
