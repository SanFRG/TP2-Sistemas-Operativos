#ifndef PROCESS_INTERNAL_H
#define PROCESS_INTERNAL_H

#include "process.h"





extern PCB process_table[MAX_PROCESSES];
extern int current_pid;
extern int idle_pid;


PCB *get_process_by_pid(int pid);



int weight_for(PCB *p);

#endif
