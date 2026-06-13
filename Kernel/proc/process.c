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

// Definidos aca pero compartidos con scheduler.c via process_internal.h.
PCB process_table[MAX_PROCESSES];//TODO:ver si mas adelante nos conviene cambiar a otra estructura
static int next_pid = 1; //0 lo dejamos como valor vacio/no asignado para el boot y etc
int current_pid = -1;

// PID del proceso idle. Solo corre cuando no hay ningun otro proceso READY,
// asi el scheduler siempre tiene algo para elegir.
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
    // Nace BLOCKED (no elegible) a proposito: el scheduler no debe poder
    // elegirlo hasta que el PCB este completo. process_create lo pasa a
    // READY al final; pcb_set_current lo pasa a RUNNING.
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
    p->sched_credits = weight_for(p);   // arranca con una ronda completa
    p->sched_vtime = 0;
    p->sem_held_count = 0;
    p->next = NULL;
}

static void init_standard_fds(PCB *p) {
    p->fd[0] = 0;  // stdin
    p->fd[1] = 1;  // stdout
    p->fd[2] = 2;  // stderr
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

// Si el padre de 'child' estaba bloqueado esperandolo con wait, lo despierta.
static void wake_parent_if_waiting(PCB *child) {
    PCB *parent = get_process_by_pid(child->parent_pid);
    if (parent != NULL && parent->state == BLOCKED &&
        parent->waiting_for_pid == child->pid) {
        parent->waiting_for_pid = 0;
        parent->state = READY;
    }
}

// Recolecta procesos muertos cuyo padre ya no existe o tambien esta muerto.
// Importante: nunca toca el proceso current, porque su stack puede estar en uso.
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

// Termina a todos los hijos vivos de 'parent_pid'. Se llama cuando un proceso
// muere (exit o kill) para que no queden huerfanos corriendo para siempre
// (p. ej. los endless_loop que crea test_proc y que nunca terminan solos).
// Los hijos READY/RUNNING se marcan kill_pending (mueren en su proximo
// checkpoint de syscall); los BLOCKED se matan en el acto, porque no van a
// llegar solos a un checkpoint. La cascada a los nietos la dispara cada hijo al
// pasar por process_exit, salvo los BLOCKED, que se propagan aca mismo.
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
            process_release_held_sems(p);  // devuelve tokens que tuviera tomados
            terminate_children(p->pid);   // cascada a los nietos
        } else {
            // READY / RUNNING: muere cooperativamente en su proximo syscall.
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
    if (pid == current_pid || pid == idle_pid) {  // el idle no se mata
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
    sem_remove_waiter(pid);   // ya protege su seccion critica con _cli/_sti
    process_release_held_sems(p);  // devuelve tokens que tuviera tomados
    terminate_children(p->pid);  // sus hijos vivos mueren con el
    wake_parent_if_waiting(p);

    // Si el proceso muerto era huerfano (padre ya no existe), nadie lo va a
    // esperar con wait: lo recolectamos aca mismo, en contexto de proceso y de
    // forma atomica (irqsave), en vez de hacerlo desde el scheduler.
    uint64_t flags = _save_irq();
    clean_orphan();
    _restore_irq(flags);
    return 0;
}

// --- Tracking de tokens de semaforo en posesion (anti-deadlock al matar) ---

void process_sem_held_push(int pid, int sem_idx) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->sem_held_count >= PROC_MAX_HELD_SEMS) {
        return;  // best-effort: si se llena, ese token no se trackea
    }
    p->sem_held[p->sem_held_count++] = sem_idx;
}

void process_sem_held_release(int pid, int sem_idx) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || p->sem_held_count <= 0) {
        return;
    }
    // Busca desde el tope el token que coincide con sem_idx (caso mutex y
    // locks anidados liberados fuera de orden). Si no hay coincidencia, saca el
    // tope: en productor/consumidor se hace wait de un semaforo y post de otro,
    // y ese post "descarga" el slot que se tenia tomado (p. ej. mvar).
    int pos = -1;
    for (int i = p->sem_held_count - 1; i >= 0; i--) {
        if (p->sem_held[i] == sem_idx) {
            pos = i;
            break;
        }
    }
    if (pos < 0) {
        pos = p->sem_held_count - 1;  // sin coincidencia: descarga el tope (LIFO)
    }
    // compacta sacando la entrada en 'pos'
    for (int i = pos; i < p->sem_held_count - 1; i++) {
        p->sem_held[i] = p->sem_held[i + 1];
    }
    p->sem_held_count--;
}

// Devuelve (postea) todos los tokens que el proceso tenia tomados y no
// devolvio. Se llama al morir, para restaurar la invariante de los semaforos y
// que el resto de los procesos no quede en deadlock. Llamar con interrupciones
// deshabilitadas o en contexto seguro; sem_force_post_index ya hace _cli/_sti.
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
        buffer[count].loop_counter = p->loop_counter;
        buffer[count].stack_pointer = (uint64_t)p->stack_pointer;
        buffer[count].base_pointer = (uint64_t)p->base_pointer;
        init_name(buffer[count].name, p->name);
        count++;
    }

    return (int)count;
}

// Termina el proceso actual de forma ordenada.
void process_exit(int exit_code) {
    PCB *me = get_process_by_pid(current_pid);
    if (me != NULL) {
        // Solo me llevo a mis hijos vivos si a MI me mataron (kill_pending).
        // Si termino ordenadamente (return / exit_process(0)), los hijos
        // sobreviven como huerfanos, igual que en Unix. Esto es clave para
        // mvar: su proceso principal spawnea writers/readers y retorna de
        // inmediato, y esos workers deben seguir corriendo.
        int was_killed = me->kill_pending;
        if (was_killed) {
            exit_code = -1;
        }
        pipe_on_process_exit(me->fd[0], me->fd[1], me->fd[2]);
        sem_remove_waiter(me->pid);
        process_release_held_sems(me);  // devuelve tokens tomados sin liberar
        me->state = TERMINATED;
        me->exit_code = exit_code;
        me->kill_pending = 0;
        if (was_killed) {
            terminate_children(me->pid);
        }
        wake_parent_if_waiting(me);   // si el padre esperaba, lo despierta
    }
    _yield();                 // cede el CPU; el scheduler ya no nos elige
    while (1) {               // por las dudas: no debe volver nunca
        _hlt();
    }
}

void process_exit_if_kill_pending(void) {
    PCB *me = get_process_by_pid(current_pid);
    if (me == NULL || !me->kill_pending) {
        return;
    }
    // Si el proceso tiene un token de semaforo tomado, posponer la muerte: que
    // termine su seccion critica y lo devuelva con sem_post antes de morir. Asi
    // no se pierde el token y no se deadlockea al resto (clave para mvar). Las
    // secciones criticas no se bloquean con un token en mano, asi que llega
    // enseguida al checkpoint siguiente con sem_held_count == 0 y muere limpio.
    if (me->sem_held_count > 0) {
        return;
    }
    process_exit(-1);
}

int process_current_kill_pending(void) {
    PCB *me = get_process_by_pid(current_pid);
    return me != NULL && me->kill_pending;
}

// Envoltorio con el que arranca TODO proceso. Llama a la funcion real y,
// si esta retorna, termina el proceso ordenadamente en vez de saltar a
// una direccion de retorno basura.
static void process_wrapper(void (*fn)(void *), void *arg) {
    fn(arg);
    process_exit(0);
}

// Crea un proceso con file descriptors a medida (fd_in/fd_out). Si fd_in o
// fd_out son < 0 se conservan los estandar (stdin/stdout) que fija
// init_process_common. process_create es el caso comun (fds estandar).
int process_create_with_fds(const char *name, void (*fn)(void *), void *arg,
                             int priority, int foreground, int fd_in, int fd_out) {
    uint64_t flags;
    PCB *p;

    if (name == NULL || fn == NULL) return -1;

    // Atomico respecto del scheduler: la busqueda de slot libre y la asignacion
    // del pid no pueden ser interrumpidas (sino dos creaciones podrian tomar el
    // mismo slot). Tambien reaprovechamos para reapear huerfanos muertos.
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
        p->pid = 0; //si falla lo dejamos como slot libre
        _restore_irq(flags);
        return -1;
    }

    // El tope del stack debe cumplir el contrato del ABI System V AMD64:
    // al entrar a la funcion del proceso debe valer rsp % 16 == 8.
    // Alineamos el tope a 16 y restamos 8 (como si un 'call' hubiera
    // empujado la direccion de retorno).
    uint64_t top = (uint64_t)p->stack_base + STACK_SIZE;
    top &= ~0xFULL;
    top -= 8;
    // El proceso arranca en process_wrapper, que recibe (funcion, arg).
    // Asi, si la funcion retorna, el wrapper llama a process_exit.
    p->stack_pointer = setProcessStackASM((void *)process_wrapper, (void *)top,
                                          (void *)fn, arg);
    p->base_pointer = p->stack_pointer;

    PCB *parent = get_process_by_pid(current_pid); //p->hijo, parent->proceso actual
    if (parent != NULL) parent->children_count++;

    // Ultimo paso: con el PCB ya completo (stack incluido), lo hacemos
    // elegible. Este unico store atomico "publica" el proceso; si un timer
    // lo interrumpe antes, lo ve BLOCKED y no lo elige a medio armar.
    p->state = READY;
    int new_pid = p->pid;
    _restore_irq(flags);
    return new_pid;
}

int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground) {
    // Caso comun: fds estandar (stdin/stdout). -1/-1 le indica a
    // process_create_with_fds que no sobrescriba los que fija init_process_common.
    return process_create_with_fds(name, function, arg, priority, foreground, -1, -1);
}

// Peso (turnos por ronda) que le corresponde a un proceso segun su prioridad.
// No lineal a proposito: la prioridad maxima recibe 3x los turnos de la media
// y 9x los de la minima, para que la diferencia se note con claridad. Lo usan
// la inicializacion/seteo de prioridad de aca y el scheduler (scheduler.c).
int weight_for(PCB *p) {
    static const int weight_table[MAX_PRIORITY + 1] = {1, 3, 9};
    int prio = p->priority;
    if (prio < MIN_PRIORITY) prio = MIN_PRIORITY;
    if (prio > MAX_PRIORITY) prio = MAX_PRIORITY;
    return weight_table[prio];
}
