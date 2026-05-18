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
    // Nace BLOCKED (no elegible) a proposito: el scheduler no debe poder
    // elegirlo hasta que el PCB este completo. process_create lo pasa a
    // READY al final; pcb_set_current lo pasa a RUNNING.
    p->state = BLOCKED;
    p->priority = priority;
    p->foreground = foreground;
    p->parent_pid = parent_pid;
    p->children_count = 0;
    p->fd[0] = 0;
    p->fd[1] = 1;
    p->fd[2] = 2;
    p->exit_code = 0;
    p->waiting_for_pid = 0;
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

// Si el padre de 'child' estaba bloqueado esperandolo con wait, lo despierta.
static void wake_parent_if_waiting(PCB *child) {
    PCB *parent = get_process_by_pid(child->parent_pid);
    if (parent != NULL && parent->state == BLOCKED &&
        parent->waiting_for_pid == child->pid) {
        parent->waiting_for_pid = 0;
        parent->state = READY;
    }
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
    p->waiting_for_pid = 0;
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
    wake_parent_if_waiting(p);
    return 0;
}

int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || pid == idle_pid) {  // al idle no se lo bloquea
        return -1;
    }
    // Solo se puede bloquear algo que corre o esta listo para correr.
    // Ya BLOCKED / KILLED / TERMINATED -> no hay nada que hacer.
    if (p->state != RUNNING) {
        return -1;
    }

    p->state = BLOCKED;

    if (pid == current_pid) {
        _yield();   // si me bloquee a mi mismo, cedo el CPU ya mismo
    }
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
    return 0;
}

int process_wait(int pid) {
    PCB *child = get_process_by_pid(pid);
    PCB *me = get_process_by_pid(current_pid);
    if (child == NULL || me == NULL) {
        return -1;
    }
    if (child->parent_pid != current_pid) {  // solo se espera a hijos propios
        return -1;
    }

    // Mientras el hijo siga vivo, me bloqueo. process_exit / process_kill
    // me pasan a READY cuando el hijo termina, y vuelvo a chequear.
    while (child->state != KILLED && child->state != TERMINATED) {
        me->waiting_for_pid = pid;
        me->state = BLOCKED;
        _yield();   // cedo el CPU; no es busy wait, quedo bloqueado
    }
    me->waiting_for_pid = 0;

    // El hijo termino: recolectar su stack y liberar su slot.
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
        init_name(buffer[count].name, p->name);
        count++;
    }

    return (int)count;
}

// Termina el proceso actual de forma ordenada.
void process_exit(int exit_code) {
    PCB *me = get_process_by_pid(current_pid);
    if (me != NULL) {
        me->state = TERMINATED;
        me->exit_code = exit_code;
        wake_parent_if_waiting(me);   // si el padre esperaba, lo despierta
    }
    _yield();                 // cede el CPU; el scheduler ya no nos elige
    while (1) {               // por las dudas: no debe volver nunca
        _yield();
    }
}

// Envoltorio con el que arranca TODO proceso. Llama a la funcion real y,
// si esta retorna, termina el proceso ordenadamente en vez de saltar a
// una direccion de retorno basura.
static void process_wrapper(void (*fn)(void *), void *arg) {
    fn(arg);
    process_exit(0);
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
    // El proceso arranca en process_wrapper, que recibe (funcion, arg).
    // Asi, si la funcion retorna, el wrapper llama a process_exit.
    p->stack_pointer = setProcessStackASM((void *)process_wrapper, stack_top,
                                          (void *)function, arg);
    p->base_pointer = p->stack_pointer;

    PCB *parent = get_process_by_pid(current_pid); //p->hijo, parent->proceso actual
    if (parent != NULL) {
        parent->children_count++;
    }

    // Ultimo paso: con el PCB ya completo (stack incluido), lo hacemos
    // elegible. Este unico store atomico "publica" el proceso; si un timer
    // lo interrumpe antes, lo ve BLOCKED y no lo elige a medio armar.
    p->state = READY;
    return p->pid;
}

/* ================= Scheduler (Round Robin con prioridades) =================
 *
 * Se recorre la tabla de procesos de forma circular (eso garantiza que
 * ningun proceso sufra starvation: a todos les toca turno en cada vuelta).
 * La prioridad no decide QUIEN corre, sino CUANTO corre: cada proceso
 * recibe (prioridad + 1) ticks del timer por turno.
 */

// Indice en process_table del proceso que corrio ultimo (punto de partida
// para la busqueda circular del round robin).
static int last_index = 0;

// Ticks de timer que le quedan al proceso actual en su turno.
static int current_quantum = 0;

// Cantidad de ticks que le corresponde a un proceso segun su prioridad.
static int quantum_for(PCB *p) {
    int prio = p->priority;
    if (prio < MIN_PRIORITY) prio = MIN_PRIORITY;
    if (prio > MAX_PRIORITY) prio = MAX_PRIORITY;
    return prio + 1;
}

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

    // 1. Guardar siempre el contexto del proceso actual.
    if (current != NULL) {
        current->stack_pointer = current_rsp;
    }

    // 2. Si el proceso actual sigue RUNNING y todavia le queda quantum,
    //    lo dejamos seguir: no hay cambio de contexto en este tick.
    if (current != NULL && current->state == RUNNING && current_quantum > 1) {
        current_quantum--;
        return current_rsp;
    }

    // 3. Se agoto el quantum (o el proceso se bloqueo / murio): hay que
    //    rotar. Si seguia RUNNING, vuelve a la cola de listos.
    if (current != NULL && current->state == RUNNING) {
        current->state = READY;
    }

    // 4. Elegir el proximo proceso READY (round robin circular).
    int idx = pick_next_ready();
    if (idx < 0) {
        // No hay ningun proceso normal listo: corre el proceso idle.
        PCB *idle = get_process_by_pid(idle_pid);
        if (idle != NULL) {
            idle->state = RUNNING;
            current_pid = idle_pid;
            current_quantum = quantum_for(idle);
            return idle->stack_pointer;
        }
        // Caso degenerado: ni siquiera existe el idle.
        return current_rsp;
    }

    // 5. Activar al elegido y asignarle su quantum segun la prioridad.
    PCB *next = &process_table[idx];
    next->state = RUNNING;
    last_index = idx;
    current_pid = next->pid;
    current_quantum = quantum_for(next);
    return next->stack_pointer;
}
