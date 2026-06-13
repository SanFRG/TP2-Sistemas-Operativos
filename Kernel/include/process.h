#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define PROCESS_NAME_LEN 32
#define MAX_PROCESSES 64




#define PROC_MAX_HELD_SEMS 32



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
    int fd[3];
    int exit_code;
    uint64_t loop_counter;
    int waiting_for_pid;
    int kill_pending;
    void *base_pointer;
    int sched_credits;
    uint64_t sched_vtime;
    int sem_held[PROC_MAX_HELD_SEMS];
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
} process_info;

void process_system_init(void);
int pcb_set_current(const char *name, int foreground, int priority, int parent_pid);
int process_get_current_pid(void);
int process_get_current_fd(int fd_index);
int process_create_with_fds(const char *name, void (*fn)(void *), void *arg, int priority, int foreground, int fd_in, int fd_out);
int process_kill(int pid);
int process_block(int pid);
int process_block_current(void);
int process_unblock(int pid);
int process_set_priority(int pid, int new_priority);






uint64_t process_get_vtime(int pid);
void process_charge_vtime(int pid);







void process_sem_held_push(int pid, int sem_idx);
void process_sem_held_release(int pid, int sem_idx);
int process_wait(int pid);
int process_list(process_info *buffer, uint64_t max_entries);
int process_create(const char *name, void (*function)(void *), void *arg, int priority, int foreground);



void process_exit(int exit_code);
void process_exit_if_kill_pending(void);
int process_current_kill_pending(void);
uint64_t process_loop_inc(void);



void *scheduler_switch(void *current_rsp);

#endif
