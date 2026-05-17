#include "process.h"
#include "memoryManager.h"
#include "lib.h"
#include "interrupts.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE 4096

static PCB process_table[MAX_PROCESSES];//TODO:ver si mas adelante nos conviene cambiar a otra estructura
static int next_pid = 1; //0 lo dejamos como valor vacio/no asignado para el boot y etc
static int current_pid = -1;

// PID del proceso idle. Solo corre cuando no hay ningun otro proceso READY,
// asi el scheduler siempre tiene algo para elegir.
static int idle_pid = -1;

static void clear_pcb_slot(PCB *p);
static void idle_process(void *arg);

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

    // Crear el proceso idle: el scheduler lo corre cuando no hay otro READY.
    idle_pid = process_create("idle", idle_process, NULL, 0, 0);
}

// Proceso idle: nunca se bloquea ni termina. Solo espera interrupciones.
static void idle_process(void *arg) {
    (void)arg;
    while (1) {
        _hlt();   // sti + hlt: habilita interrupciones y espera la proxima
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
    if (pid == current_pid || pid == idle_pid) {  // el idle no se mata
        return -1;
    }

    p->state = KILLED;
    p->exit_code = -1;
    return 0;
}

int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    
    if (p == NULL || p->state != RUNNING) {//preguntar
        return -1;
    }

    p->state = BLOCKED;

    if (pid == current_pid) {
        _yield();
    }

    return 0;
}

/*
int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) return -1;
    if (p->state != READY && p->state != RUNNING) return -1; // no bloquear KILLED/TERMINATED/ya BLOCKED
    p->state = BLOCKED;
    // si bloqueaste al proceso actual, hay que ceder el CPU al scheduler
    if (pid == current_pid) {
        yield();
    }
    return 0;
}

*/

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
        if (p->pid == 0 || p->state == KILLED) {
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

    // El tope del stack debe cumplir el contrato del ABI System V AMD64:
    // al entrar a la funcion del proceso debe valer rsp % 16 == 8.
    // Alineamos el tope a 16 y restamos 8 (como si un 'call' hubiera
    // empujado la direccion de retorno).
    uint64_t top = (uint64_t)p->stack_base + STACK_SIZE;
    top &= ~0xFULL;
    top -= 8;
    void *stack_top = (void *)top;
    p->stack_pointer = setProcessStackASM((void *)function, stack_top, arg);
    p->base_pointer = p->stack_pointer;

    PCB *parent = get_process_by_pid(current_pid); //p->hijo, parent->proceso actual
    if (parent != NULL) {
        parent->children_count++;
    }

    return p->pid;
}

/* ===================== Scheduler (Round Robin simple) =====================
 *
 * Round Robin sin prioridades: se recorre la tabla de procesos de forma
 * circular y se le da el CPU al proximo proceso en estado READY.
 */

// Indice en process_table del proceso que corrio ultimo (punto de partida
// para la busqueda circular del round robin).
static int last_index = 0;

// Devuelve el indice del proximo proceso READY recorriendo la tabla de
// forma circular a partir del que corrio ultimo. -1 si no hay ninguno.
static int pick_next_ready(void) {
    for (int offset = 1; offset <= MAX_PROCESSES; offset++) {
        int idx = (last_index + offset) % MAX_PROCESSES;
        PCB *p = &process_table[idx];
        // El idle se ignora aca: solo se elige como ultimo recurso.
        if (p->pid != 0 && p->pid != idle_pid && p->state == READY) {
            return idx;
        }
    }
    return -1;
}

// Llamada desde el handler del timer (IRQ0) en interrupts.asm.
// Recibe el rsp del proceso interrumpido (que apunta a su contexto ya
// guardado en el stack) y devuelve el rsp del proximo proceso a correr.
void *scheduler_switch(void *current_rsp) {
    PCB *current = get_process_by_pid(current_pid);

    // 1. Guardar el contexto del proceso que venia corriendo.
    //    El contexto YA esta en el stack; solo guardamos el puntero.
    if (current != NULL) {
        current->stack_pointer = current_rsp;
        if (current->state == RUNNING) {
            current->state = READY;   // vuelve a la cola de listos
        }
        // Si quedo BLOCKED/KILLED/TERMINATED se respeta ese estado.
    }

    // 2. Elegir el proximo proceso READY.
    int idx = pick_next_ready();
    if (idx < 0) {
        // No hay ningun proceso normal listo: corre el proceso idle.
        PCB *idle = get_process_by_pid(idle_pid);
        if (idle != NULL) {
            idle->state = RUNNING;
            current_pid = idle_pid;
            return idle->stack_pointer;
        }
        // Caso degenerado: ni siquiera existe el idle.
        return current_rsp;
    }

    // 3. Activar al elegido y devolver su rsp guardado.
    PCB *next = &process_table[idx];
    next->state = RUNNING;
    last_index = idx;
    current_pid = next->pid;
    return next->stack_pointer;
}
