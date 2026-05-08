#ifndef PROCESS_H
#define PROCESS_H

typedef enum { READY, RUNNING, BLOCKED, KILLED } ProcessState;

typedef struct Process {
    int pid;
    char name[32];
    ProcessState state;
    int priority;
    void *stack_base;
    void *stack_pointer;
    int foreground;
    int parent_pid;
    void *base_pointer;
    struct Process *next;
} Process;

Process *create_process(char *name, void (*function)(void *), void *arg, int priority);

#endif