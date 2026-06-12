#include "process.h"
#include "memoryManager.h"
#include "lib.h"
#include "interrupts.h"
#include "semaphore.h"
#include "pipe.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_SIZE 16384

static PCB process_table[MAX_PROCESSES];//TODO:ver si mas adelante nos conviene cambiar a otra estructura
static int next_pid = 1; //0 lo dejamos como valor vacio/no asignado para el boot y etc
static int current_pid = -1;

// PID del proceso idle. Solo corre cuando no hay ningun otro proceso READY,
// asi el scheduler siempre tiene algo para elegir.
static int idle_pid = -1;

static void clear_pcb_slot(PCB *p);
static void idle_process(void *arg);
static void clean_orphan(void);
static void init_standard_fds(PCB *p);
static int weight_for(PCB *p);

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
    p->next = NULL;
}

static void init_standard_fds(PCB *p) {
    p->fd[0] = 0;  // stdin
    p->fd[1] = 1;  // stdout
    p->fd[2] = 2;  // stderr
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

int process_get_current_fd(int fd_index) {
    if (fd_index < 0 || fd_index >= 3) return -1;
    PCB *p = get_process_by_pid(current_pid);
    if (p == NULL) return -1;
    return p->fd[fd_index];
}

int process_get_fd(int pid, int fd_index) {
    if (fd_index < 0 || fd_index >= 3) return -1;
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) return -1;
    return p->fd[fd_index];
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

int process_block(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL || pid == idle_pid) {  // al idle no se lo bloquea
        return -1;
    }
    // Solo se puede bloquear algo que corre o esta listo para correr.
    // Ya BLOCKED / KILLED / TERMINATED -> no hay nada que hacer.
    if (p->state != READY && p->state != RUNNING) {
        return -1;
    }

    p->state = BLOCKED;

    if (pid == current_pid) {
        _yield();   // si me bloquee a mi mismo, cedo el CPU ya mismo
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
    // Reflejar el nuevo peso ya mismo: le damos una ronda completa con la
    // prioridad recien seteada para que el efecto se vea sin esperar al
    // proximo refill.
    p->sched_credits = weight_for(p);
    return 0;
}

int process_get_priority(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return -1;
    }
    return p->priority;
}

uint64_t process_get_vtime(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return ~0ULL;   // inexistente: vtime maximo -> nunca es el minimo
    }
    return p->sched_vtime;
}

void process_charge_vtime(int pid) {
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) {
        return;
    }
    // Avance = 9/peso. Con pesos {1,3,9} da {9,3,1}: a mayor prioridad, menor
    // avance -> el vtime crece mas lento -> es elegido mas seguido. El 9 es el
    // peso maximo, asi que el avance minimo es 1 (sin division por cero).
    p->sched_vtime += (uint64_t)(9 / weight_for(p));
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
    if (me != NULL && me->kill_pending) {
        process_exit(-1);
    }
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

int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground) {
    uint64_t flags;
    PCB *p;

    if (name == NULL || function == NULL) {
        return -1;
    }

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
    int new_pid = p->pid;
    _restore_irq(flags);
    return new_pid;
}

uint64_t process_loop_inc(void) { 
    PCB *me = get_process_by_pid(current_pid);
    if (me == NULL) return -1;
    return ++me->loop_counter;
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

/* ============== Scheduler (Round Robin ponderado por prioridad) ==============
 *
 * La prioridad decide CON QUE FRECUENCIA se elige un proceso, no cuanto corre
 * seguido. Cada proceso recibe, por ronda, una cantidad de "creditos" (turnos)
 * igual a su peso: {1, 3, 9} segun prioridad 0/1/2. En cada tick del timer se
 * elige -en orden circular- un proceso READY que aun tenga creditos y se corre
 * un tick; al elegirlo se le descuenta un credito. Cuando ningun READY tiene
 * creditos, se recargan todos (nueva ronda).
 *
 * Por que asi y no con quantum mas largo: un proceso que se BLOQUEA antes de
 * agotar su quantum (p. ej. los writers de mvar, que escriben y se duermen en
 * el semaforo) no saca ningun provecho de un quantum largo: cede el CPU
 * enseguida igual. Lo unico que lo hace "aparecer" mas seguido es ser ELEGIDO
 * mas veces. Ponderar la frecuencia logra eso de forma independiente del
 * hardware y de cuanto dure el busy-wait. Para procesos CPU-bound (test_prio)
 * el efecto es el mismo de siempre: a mayor prioridad, mas CPU, termina antes.
 */

// Indice en process_table del proceso que corrio ultimo (punto de partida
// para la busqueda circular del round robin).
static int last_index = 0;

// Peso (turnos por ronda) que le corresponde a un proceso segun su prioridad.
// No lineal a proposito: la prioridad maxima recibe 3x los turnos de la media
// y 9x los de la minima, para que la diferencia se note con claridad.
static int weight_for(PCB *p) {
    static const int weight_table[MAX_PRIORITY + 1] = {1, 3, 9};
    int prio = p->priority;
    if (prio < MIN_PRIORITY) prio = MIN_PRIORITY;
    if (prio > MAX_PRIORITY) prio = MAX_PRIORITY;
    return weight_table[prio];
}

// Recarga los creditos de todos los procesos vivos segun su peso (nueva ronda).
static void refill_credits(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        PCB *p = &process_table[i];
        if (p->pid != 0 && p->pid != idle_pid) {
            p->sched_credits = weight_for(p);
        }
    }
}

// Devuelve el indice del proximo proceso READY con creditos, recorriendo la
// tabla de forma circular desde el que corrio ultimo. Si nadie tiene creditos
// pero hay procesos READY, recarga (nueva ronda) y reintenta. -1 si no hay
// ningun proceso normal listo.
static int pick_next_ready(void) {
    for (int pass = 0; pass < 2; pass++) {
        for (int offset = 1; offset <= MAX_PROCESSES; offset++) {
            int idx = (last_index + offset) % MAX_PROCESSES;
            PCB *p = &process_table[idx];
            // El idle se ignora aca: solo se elige como ultimo recurso.
            if (p->pid != 0 && p->pid != idle_pid && p->state == READY &&
                p->sched_credits > 0) {
                return idx;
            }
        }
        // Nadie listo con creditos: arrancar una ronda nueva y reintentar.
        refill_credits();
    }
    return -1;
}

// Llamada desde el handler del timer (IRQ0) en interrupts.asm.
// Recibe el rsp del proceso interrumpido (que apunta a su contexto ya
// guardado en el stack) y devuelve el rsp del proximo proceso a correr.
void *scheduler_switch(void *current_rsp) {
    PCB *current = get_process_by_pid(current_pid);

    // NOTA: la recoleccion de huerfanos (clean_orphan) NO se hace aca. Antes se
    // llamaba en cada tick, pero liberar memoria (mm_free) y limpiar slots desde
    // el contexto de interrupcion se pisaba con las operaciones de proceso
    // (create/kill/wait) y corrompia el heap/tabla -> #GP. Ahora se reapean los
    // huerfanos en contexto de proceso (process_kill / process_create), de forma
    // atomica con irqsave.

    // 1. Guardar el contexto del proceso actual y devolverlo a la cola de
    //    listos si seguia corriendo (cada turno dura un tick).
    if (current != NULL) {
        current->stack_pointer = current_rsp;
        if (current->state == RUNNING) {
            current->state = READY;
        }
    }

    // 2. Elegir el proximo proceso READY con creditos (round robin ponderado).
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

    // 3. Activar al elegido y consumirle un turno de la ronda.
    PCB *next = &process_table[idx];
    next->state = RUNNING;
    next->sched_credits--;
    last_index = idx;
    current_pid = next->pid;
    return next->stack_pointer;
}
