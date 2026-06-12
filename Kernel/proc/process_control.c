/* Accesores y control sobre un proceso ya existente: consultas de pid/fd,
 * block/unblock, prioridad, tiempo virtual y loop_counter. Separado de
 * process.c (que maneja el ciclo de vida: creacion, terminacion y reaping)
 * porque estas operaciones solo tocan campos del PCB y dependen unicamente del
 * estado compartido del modulo (process_internal.h): no agregan acoplamiento. */
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

int process_set_current_fd(int fd_index, int fd_value) {
    if (fd_index < 0 || fd_index >= 3 || fd_value < 0) return -1;
    PCB *p = get_process_by_pid(current_pid);
    if (p == NULL) return -1;
    p->fd[fd_index] = fd_value;
    return 0;
}

int process_get_fd(int pid, int fd_index) {
    if (fd_index < 0 || fd_index >= 3) return -1;
    PCB *p = get_process_by_pid(pid);
    if (p == NULL) return -1;
    return p->fd[fd_index];
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

uint64_t process_loop_inc(void) {
    PCB *me = get_process_by_pid(current_pid);
    if (me == NULL) return -1;
    return ++me->loop_counter;
}
