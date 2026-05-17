#include "process.h"
#include "memoryManager.h"
#include "lib.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE 4096

static PCB process_table[MAX_PROCESSES];//TODO:ver si mas adelante nos conviene cambiar a otra estructura
static int next_pid = 1; //0 lo dejamos como valor vacio/no asignado para el boot y etc
static int current_pid = -1;

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
    p->state = READY;
    p->priority = priority;
    p->foreground = foreground;
    p->parent_pid = parent_pid;
    p->children_count = 0;
    p->fd[0] = 0;
    p->fd[1] = 1;
    p->fd[2] = 2;
    p->exit_code = 0;
    p->stack_base = NULL;
    p->stack_pointer = NULL;
    p->base_pointer = NULL;
    p->next = NULL;
}

static PCB *get_process_by_pid(int pid) {
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

void process_system_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        clear_pcb_slot(&process_table[i]);
    }
    next_pid = 1;
    current_pid = -1; 
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
    p->base_pointer = NULL;
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

int process_get_current_pid(void) {
    return current_pid;
}

int process_kill(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->state == KILLED) {
        return -1;
    }
    if (pid == current_pid) {
        return -1;
    }

    p->state = KILLED;
    p->exit_code = -1;
    return 0;
}

int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->state != RUNNING) {
        return -1;
    }
    if (pid == current_pid) {
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
    if (p == NULL || p->state == KILLED || new_priority < 0) {
        return -1;
    }
    p->priority = new_priority;
    return 0;
}

int process_wait(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return -1;
    }
    if (current_pid != -1 && p->parent_pid != current_pid) { //valido que sea un proceso padre para hacer wait
        return -1;
    }
    if (p->state != KILLED && p->state != TERMINATED) { //si el hijo sigue vivo retorno -1, si terminó ahi recien hago el clean
        return -1; 
    }

    if (p->stack_base != NULL) {
        mm_free(p->stack_base);
    }

    PCB *parent = get_process_by_pid(p->parent_pid);
    if (parent != NULL && parent->children_count > 0) {
        parent->children_count--;
    }

    clear_pcb_slot(p);
    return 0;
}

int process_list(process_info *buffer, uint64_t max_entries) {
    if (buffer == NULL || max_entries == 0) {
        return -1;
    }

    uint64_t count = 0;
    for (int i = 0; i < MAX_PROCESSES && count < max_entries; i++) {
        PCB *p = &process_table[i];
        if (p->pid == 0 || p->state == KILLED) { TODO: mejorar codigo
            continue;
        }

        buffer[count].pid = p->pid;
        buffer[count].parent_pid = p->parent_pid;
        buffer[count].priority = p->priority;
        buffer[count].foreground = p->foreground;
        buffer[count].state = (int)p->state;
        init_name(buffer[count].name, p->name);
        count++;
    }

    return (int)count;
}

int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground) {
    if (name == NULL || function == NULL) {
        return -1;
    }

    PCB *p = get_free_slot();
    if (p == NULL) {
        return -1;
    }

    init_process_common(p, name, foreground ? 1 : 0, priority, current_pid);

    p->stack_base = mm_alloc(STACK_SIZE);
    if (p->stack_base == NULL) {
        p->pid = 0; //si falla lo dejamos como slot libre
        return -1;
    }

    void *stack_top = (void *)((uint8_t *)p->stack_base + STACK_SIZE);
    p->stack_pointer = setProcessStackASM((void *)function, stack_top, arg);
    p->base_pointer = p->stack_pointer;

    PCB *parent = get_process_by_pid(current_pid); //p->hijo, parent->proceso actual
    if (parent != NULL) {
        parent->children_count++;
    }


    return p->pid;
}
