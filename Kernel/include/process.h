#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROCESS_NAME_LEN 32
#define MAX_PROCESSES 64

// Capacidad del stack de "tokens de semaforo en posesion" de cada proceso.
// Se usa para devolver automaticamente los tokens al morir y evitar deadlocks
// (ver process_release_held_sems / semaphore.c).
#define PROC_MAX_HELD_SEMS 32

// Rango de prioridades del scheduler. Mayor numero = mas prioridad =
// elegido con mas frecuencia por el scheduler (mas turnos por ronda).
#define MIN_PRIORITY 0
#define MAX_PRIORITY 2
#define DEFAULT_PRIORITY 1

typedef enum { READY, RUNNING, BLOCKED, KILLED, TERMINATED } ProcessState;

typedef struct PCB {
    int pid;
    char name[PROCESS_NAME_LEN];
    ProcessState state;
    int priority;
    void *stack_base;
    void *stack_pointer;
    int foreground;
    int parent_pid;
    int children_count;
    int fd[3];             // 0=stdin, 1=stdout, 2=stderr; base para pipes
    int exit_code;
    uint64_t loop_counter;
    int waiting_for_pid;   // PID del hijo que este proceso espera con wait (0 = ninguno)
    int kill_pending;
    void *base_pointer;
    int sched_credits;       // turnos restantes en la ronda actual (weighted RR)
    uint64_t sched_vtime;    // tiempo virtual para wakeup justo ponderado de semaforos
    int sem_held[PROC_MAX_HELD_SEMS]; // stack de indices de semaforo tomados y no devueltos
    int sem_held_count;
    struct PCB *next;
} PCB;

typedef struct {
    int pid;
    int parent_pid;
    int priority;
    int foreground;
    int state;
    uint64_t loop_counter;
    uint64_t stack_pointer;
    uint64_t base_pointer;
    char name[PROCESS_NAME_LEN];
} process_info; //separado más que nada por el ps y evitar que se acceda a info clave desde userland..TODO:ver si quitar o no

void process_system_init(void);
int pcb_set_current(const char *name, int foreground, int priority, int parent_pid);
int process_get_current_pid(void);
int process_get_current_fd(int fd_index);
int process_create_with_fds(const char *name, void (*fn)(void *), void *arg, int priority, int foreground, int fd_in, int fd_out);
int process_kill(int pid);
int process_block(int pid);
int process_block_current(void);  // Usado por semaforos para dormir al proceso actual
int process_unblock(int pid);
int process_set_priority(int pid, int new_priority);

// Tiempo virtual para wakeup justo ponderado (weighted fair queueing) en los
// semaforos. process_get_vtime devuelve el vtime actual del proceso (un valor
// muy grande si el pid no existe, para que nunca sea elegido). process_charge_vtime
// avanza el vtime del proceso en 9/peso, de modo que a mayor prioridad avanza
// mas lento y por ende es despertado mas seguido (pero sin monopolizar).
uint64_t process_get_vtime(int pid);
void process_charge_vtime(int pid);

// Tracking de tokens de semaforo en posesion de un proceso, para devolverlos
// automaticamente si el proceso muere en su seccion critica (evita deadlocks).
// Las llama semaphore.c: push al tomar un token (sem_wait exitoso), release al
// devolverlo (sem_post). release saca el token que coincide con sem_idx; si no
// hay coincidencia (patron productor/consumidor, p. ej. mvar: se hace wait de
// uno y post de otro) saca el tope del stack (LIFO).
void process_sem_held_push(int pid, int sem_idx);
void process_sem_held_release(int pid, int sem_idx);
int process_wait(int pid);
int process_list(process_info *buffer, uint64_t max_entries);
int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground);

// Termina el proceso actual de forma ordenada. La llama el wrapper cuando
// la funcion del proceso retorna; tambien sirve como syscall de exit.
void process_exit(int exit_code);
void process_exit_if_kill_pending(void);
int process_current_kill_pending(void);
uint64_t process_loop_inc(void);

// Cambio de contexto: recibe el rsp del proceso interrumpido y devuelve
// el rsp del proximo proceso a correr. La invoca el handler del timer.
void *scheduler_switch(void *current_rsp);

#endif
