#include "process.h"
#include "process_internal.h"
#include <stddef.h>

static int last_index = 0;

static void refill_credits(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &process_table[i];
        if (p->pid != 0 && p->pid != idle_pid) {
            p->sched_credits = weight_for(p);
        }
    }
}

static int pick_next_ready(void) {
    for (int pass = 0; pass < 2; pass++) {
        for (int offset = 1; offset <= MAX_PROCESSES; offset++) {
            int idx = (last_index + offset) % MAX_PROCESSES;
            PCB *p = &process_table[idx];
            if (p->pid != 0 && p->pid != idle_pid && p->state == READY &&
                p->sched_credits > 0) {
                return idx;
            }
        }
        refill_credits();
    }
    return -1;
}

void *scheduler_switch(void *current_rsp) {
    PCB *current = get_process_by_pid(current_pid);

    if (current != NULL) {
        current->stack_pointer = current_rsp;
        if (current->state == RUNNING) {
            current->state = READY;
        }
    }

    int idx = pick_next_ready();
    if (idx < 0) {
        PCB *idle = get_process_by_pid(idle_pid);
        if (idle != NULL) {
            idle->state = RUNNING;
            current_pid = idle_pid;
            return idle->stack_pointer;
        }
        return current_rsp;
    }

    PCB *next = &process_table[idx];
    next->state = RUNNING;
    next->sched_credits--;
    last_index = idx;
    current_pid = next->pid;
    return next->stack_pointer;
}
