#include "process.h"
#include "process_internal.h"
#include "interrupts.h"
#include <stddef.h>

int process_get_current_pid(void) {
    return current_pid;
}

int process_get_current_fd(int fd_index) {
    if (fd_index < 0 || fd_index >= 3) return -1;
    PCB *p = get_process_by_pid(current_pid);
    if (p == NULL) return -1;
    return p->fd[fd_index];
}

int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || pid == idle_pid) {
        return -1;
    }
    if (p->state != READY && p->state != RUNNING) {
        return -1;
    }

    p->state = BLOCKED;

    if (pid == current_pid) {
        _yield();
    }
    return 0;
}

int process_block_current(void) {
    PCB *p = get_process_by_pid(current_pid);
    if (p == NULL || current_pid == idle_pid) {
        return -1;
    }
    if (p->state != READY && p->state != RUNNING) {
        return -1;
    }
    p->state = BLOCKED;
    return 0;
}

int process_unblock(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->state != BLOCKED) {
        return -1;
    }
    p->state = READY;
    return 0;
}

int process_set_priority(int pid, int new_priority) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->state == KILLED) {
        return -1;
    }
    if (new_priority < MIN_PRIORITY) {
        new_priority = MIN_PRIORITY;
    } else if (new_priority > MAX_PRIORITY) {
        new_priority = MAX_PRIORITY;
    }
    p->priority = new_priority;
    p->sched_credits = weight_for(p);
    return 0;
}

uint64_t process_get_vtime(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return ~0ULL;
    }
    return p->sched_vtime;
}

void process_charge_vtime(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return;
    }
    p->sched_vtime += (uint64_t)(9 / weight_for(p));
}

uint64_t process_loop_inc(void) {
    PCB *me = get_process_by_pid(current_pid);
    if (me == NULL) return -1;
    return ++me->loop_counter;
}
