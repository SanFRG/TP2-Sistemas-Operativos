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
 *
 * El estado que recorre (process_table, current_pid, idle_pid) y los helpers
 * get_process_by_pid / weight_for son del modulo proc/ y se comparten con
 * process.c via process_internal.h.
 */

#include "process.h"
#include "process_internal.h"
#include <stddef.h>

// Indice en process_table del proceso que corrio ultimo (punto de partida
// para la busqueda circular del round robin).
static int last_index = 0;

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
