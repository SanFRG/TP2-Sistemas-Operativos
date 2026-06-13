#include "process.h"
#include "process_internal.h"
#include "memoryManager.h"
#include "lib.h"
#include "interrupts.h"
#include "semaphore.h"
#include "pipe.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE 16384

PCB process_table[MAX_PROCESSES];
static int next_pid = 1;
int current_pid = -1;

int idle_pid = -1;

static void clear_pcb_slot(PCB *p);
static void process_release_held_sems(PCB *p);
static void idle_process(void *arg);
static void clean_orphan(void);
static void init_standard_fds(PCB *p);

static int generate_pid(void) {
    return next_pid++;
}


static void init_name(char *dst, const char *src) {
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, PROCESS_NAME_LEN - 1);
    dst[PROCESS_NAME_LEN - 1] = '\0';
}

static void init_process_common(PCB *p, const char *name, int foreground, int priority, int parent_pid) {
    p->pid = generate_pid();
    init_name(p->name, name);
    p->state = BLOCKED;
    p->priority = priority;
    p->foreground = foreground;
    p->parent_pid = parent_pid;
    p->children_count = 0;
    init_standard_fds(p);
    p->exit_code = 0;
    p->waiting_for_pid = 0;
    p->kill_pending = 0;
    p->loop_counter = 0;
    p->stack_base = NULL;
    p->stack_pointer = NULL;
    p->base_pointer = NULL;
    p->sched_credits = weight_for(p);
    p->sched_vtime = 0;
    p->sem_held_count = 0;
    p->next = NULL;
}

static void init_standard_fds(PCB *p) {
    p->fd[0] = 0;
    p->fd[1] = 1;
    p->fd[2] = 2;
}

PCB *get_process_by_pid(int pid) {
    if (pid <= 0) {
        return NULL;
    }
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

static PCB *get_free_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            return &process_table[i];
        }
    }
    return NULL;
}

static void wake_parent_if_waiting(PCB *child) {
    PCB *parent = get_process_by_pid(child->parent_pid);
    if (parent != NULL && parent->state == BLOCKED &&
        parent->waiting_for_pid == child->pid) {
        parent->waiting_for_pid = 0;
        parent->state = READY;
    }
}

static void clean_orphan(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &process_table[i];
        PCB *parent;

        if (p->pid == 0 || p->pid == idle_pid || p->pid == current_pid) {
            continue;
        }
        if (p->state != KILLED && p->state != TERMINATED) {
            continue;
        }

        parent = get_process_by_pid(p->parent_pid);
        if (p->parent_pid <= 0 || parent == NULL ||
            parent->state == KILLED || parent->state == TERMINATED) {
            if (p->stack_base != NULL) {
                mm_free(p->stack_base);
            }
            clear_pcb_slot(p);
        }
    }
}

static void terminate_children(int parent_pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &process_table[i];
        if (p->pid == 0 || p->pid == idle_pid || p->pid == parent_pid) {
            continue;
        }
        if (p->parent_pid != parent_pid) {
            continue;
        }
        if (p->state == KILLED || p->state == TERMINATED) {
            continue;
        }

        if (p->state == BLOCKED) {
            pipe_on_process_exit(p->fd[0], p->fd[1], p->fd[2]);
            p->state = KILLED;
            p->exit_code = -1;
            p->kill_pending = 0;
            sem_remove_waiter(p->pid);
            process_release_held_sems(p);
            terminate_children(p->pid);
        } else {
            p->kill_pending = 1;
            p->exit_code = -1;
        }
    }
}

void process_system_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        clear_pcb_slot(&process_table[i]);
    }
    next_pid = 1;
    current_pid = -1;

    idle_pid = process_create("idle", idle_process, NULL, 0, 0);
}

static void idle_process(void *arg) {
    (void)arg;
    while (1) {
        _hlt();
    }
}

static void clear_pcb_slot(PCB *p) {
    if (p == NULL) {
        return;
    }

    p->pid = 0;
    p->name[0] = '\0';
    p->state = TERMINATED;
    p->priority = 0;
    p->stack_base = NULL;
    p->stack_pointer = NULL;
    p->foreground = 0;
    p->parent_pid = 0;
    p->children_count = 0;
    p->fd[0] = 0;
    p->fd[1] = 0;
    p->fd[2] = 0;
    p->exit_code = 0;
    p->waiting_for_pid = 0;
    p->kill_pending = 0;
    p->loop_counter = 0;
    p->base_pointer = NULL;
    p->sched_credits = 0;
    p->sched_vtime = 0;
    p->sem_held_count = 0;
    p->next = NULL;
}

int pcb_set_current(const char *name, int foreground, int priority, int parent_pid) {
    PCB *p = get_free_slot();
    if (p == NULL) {
        return -1;
    }

    init_process_common(p, name, foreground, priority, parent_pid);
    p->state = RUNNING;
    current_pid = p->pid;
    return p->pid;
}

int process_kill(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->state == KILLED || p->state == TERMINATED) {
        return -1;
    }
    if (pid == current_pid || pid == idle_pid) {
        return -1;
    }

    if (p->state == READY || p->state == RUNNING) {
        p->kill_pending = 1;
        p->exit_code = -1;
        return 0;
    }

    pipe_on_process_exit(p->fd[0], p->fd[1], p->fd[2]);
    p->state = KILLED;
    p->exit_code = -1;
    p->kill_pending = 0;
    sem_remove_waiter(pid);
    process_release_held_sems(p);
    terminate_children(p->pid);
    wake_parent_if_waiting(p);

    uint64_t flags = _save_irq();
    clean_orphan();
    _restore_irq(flags);
    return 0;
}

void process_sem_held_push(int pid, int sem_idx) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->sem_held_count >= PROC_MAX_HELD_SEMS) {
        return;
    }
    p->sem_held[p->sem_held_count++] = sem_idx;
}

void process_sem_held_release(int pid, int sem_idx) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->sem_held_count <= 0) {
        return;
    }
    int pos = -1;
    for (int i = p->sem_held_count - 1; i >= 0; i--) {
        if (p->sem_held[i] == sem_idx) {
            pos = i;
            break;
        }
    }
    if (pos < 0) {
        pos = p->sem_held_count - 1;
    }
    for (int i = pos; i < p->sem_held_count - 1; i++) {
        p->sem_held[i] = p->sem_held[i + 1];
    }
    p->sem_held_count--;
}

static void process_release_held_sems(PCB *p) {
    if (p == NULL) {
        return;
    }
    while (p->sem_held_count > 0) {
        int idx = p->sem_held[--p->sem_held_count];
        sem_force_post_index(idx);
    }
}

int process_wait(int pid) {
    PCB *child = get_process_by_pid(pid);
    PCB *me = get_process_by_pid(current_pid);
    if (child == NULL || me == NULL) {
        return -1;
    }
    if (child->parent_pid != current_pid) {
        return -1;
    }

    while (child->state != KILLED && child->state != TERMINATED) {
        me->waiting_for_pid = pid;
        me->state = BLOCKED;
        _yield();
    }
    me->waiting_for_pid = 0;

    int code = child->exit_code;
    if (child->stack_base != NULL) {
        mm_free(child->stack_base);
    }
    if (me->children_count > 0) {
        me->children_count--;
    }
    clear_pcb_slot(child);
    return code;
}

int process_list(process_info *buffer, uint64_t max_entries) {
    if (buffer == NULL || max_entries == 0) {
        return -1;
    }

    uint64_t count = 0;
    for (int i = 0; i < MAX_PROCESSES && count < max_entries; i++) {
        PCB *p = &process_table[i];
        if (p->pid == 0 || p->state == KILLED) {
            continue;
        }

        buffer[count].pid = p->pid;
        buffer[count].parent_pid = p->parent_pid;
        buffer[count].priority = p->priority;
        buffer[count].foreground = p->foreground;
        buffer[count].state = (int)p->state;
        buffer[count].loop_counter = p->loop_counter;
        buffer[count].stack_pointer = (uint64_t)p->stack_pointer;
        buffer[count].base_pointer = (uint64_t)p->base_pointer;
        init_name(buffer[count].name, p->name);
        count++;
    }

    return (int)count;
}

void process_exit(int exit_code) {
    PCB *me = get_process_by_pid(current_pid);
    if (me != NULL) {
        int was_killed = me->kill_pending;
        if (was_killed) {
            exit_code = -1;
        }
        pipe_on_process_exit(me->fd[0], me->fd[1], me->fd[2]);
        sem_remove_waiter(me->pid);
        process_release_held_sems(me);
        me->state = TERMINATED;
        me->exit_code = exit_code;
        me->kill_pending = 0;
        if (was_killed) {
            terminate_children(me->pid);
        }
        wake_parent_if_waiting(me);
    }
    _yield();
    while (1) {
        _hlt();
    }
}

void process_exit_if_kill_pending(void) {
    PCB *me = get_process_by_pid(current_pid);
    if (me == NULL || !me->kill_pending) {
        return;
    }
    if (me->sem_held_count > 0) {
        return;
    }
    process_exit(-1);
}

int process_current_kill_pending(void) {
    PCB *me = get_process_by_pid(current_pid);
    return me != NULL && me->kill_pending;
}

static void process_wrapper(void (*fn)(void *), void *arg) {
    fn(arg);
    process_exit(0);
}

int process_create_with_fds(const char *name, void (*fn)(void *), void *arg,
                             int priority, int foreground, int fd_in, int fd_out) {
    uint64_t flags;
    PCB *p;

    if (name == NULL || fn == NULL) return -1;

    flags = _save_irq();
    clean_orphan();

    p = get_free_slot();
    if (p == NULL) {
        _restore_irq(flags);
        return -1;
    }

    init_process_common(p, name, foreground ? 1 : 0, priority, current_pid);

    if (fd_in >= 0)  p->fd[0] = fd_in;
    if (fd_out >= 0) p->fd[1] = fd_out;

    p->stack_base = mm_alloc(STACK_SIZE);
    if (p->stack_base == NULL) {
        p->pid = 0;
        _restore_irq(flags);
        return -1;
    }

    uint64_t top = (uint64_t)p->stack_base + STACK_SIZE;
    top &= ~0xFULL;
    top -= 8;
    p->stack_pointer = setProcessStackASM((void *)process_wrapper, (void *)top,
                                          (void *)fn, arg);
    p->base_pointer = p->stack_pointer;

    PCB *parent = get_process_by_pid(current_pid);
    if (parent != NULL) parent->children_count++;

    p->state = READY;
    int new_pid = p->pid;
    _restore_irq(flags);
    return new_pid;
}

int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground) {
    return process_create_with_fds(name, function, arg, priority, foreground, -1, -1);
}

int weight_for(PCB *p) {
    static const int weight_table[MAX_PRIORITY + 1] = {1, 3, 9};
    int prio = p->priority;
    if (prio < MIN_PRIORITY) prio = MIN_PRIORITY;
    if (prio > MAX_PRIORITY) prio = MAX_PRIORITY;
    return weight_table[prio];
}
