#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROCESS_NAME_LEN 32
#define MAX_PROCESSES 64

// Rango de prioridades del scheduler. Mayor numero = mas prioridad =
// mas ticks de CPU por turno.
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
    void *base_pointer;
    struct PCB *next;
} PCB;

typedef struct {
    int pid;
    int parent_pid;
    int priority;
    int foreground;
    int state;
    uint64_t loop_counter;
    char name[PROCESS_NAME_LEN];
} process_info; //separado más que nada por el ps y evitar que se acceda a info clave desde userland..TODO:ver si quitar o no

void process_system_init(void);
int pcb_set_current(const char *name, int foreground, int priority, int parent_pid);
int process_get_current_pid(void);
int process_get_current_fd(int fd_index);
int process_get_fd(int pid, int fd_index);
int process_create_with_fds(const char *name, void (*fn)(void *), void *arg, int priority, int foreground, int fd_in, int fd_out);
int process_kill(int pid);
int process_block(int pid);
int process_block_current(void);  // Usado por semaforos para dormir al proceso actual
int process_unblock(int pid);
int process_set_priority(int pid, int new_priority);
int process_wait(int pid);
int process_list(process_info *buffer, uint64_t max_entries);
int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground);

// Termina el proceso actual de forma ordenada. La llama el wrapper cuando
// la funcion del proceso retorna; tambien sirve como syscall de exit.
void process_exit(int exit_code);
uint64_t process_loop_inc(void);

// Cambio de contexto: recibe el rsp del proceso interrumpido y devuelve
// el rsp del proximo proceso a correr. La invoca el handler del timer.
void *scheduler_switch(void *current_rsp);

#endif
