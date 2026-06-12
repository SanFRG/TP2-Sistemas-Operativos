#ifndef PROCESS_INTERNAL_H
#define PROCESS_INTERNAL_H

#include "process.h"

// Estado interno del modulo proc/, compartido entre process.c (dueno del estado)
// y scheduler.c (lo recorre en cada tick). NO es API publica: solo deben
// incluir este header esos dos archivos. El resto del kernel usa process.h.

extern PCB process_table[MAX_PROCESSES];
extern int current_pid;   // PID del proceso en RUNNING; el scheduler lo actualiza
extern int idle_pid;      // PID del proceso idle (solo corre si no hay otro READY)

// Busca un PCB por pid en la tabla. NULL si no existe (pid <= 0 incluido).
PCB *get_process_by_pid(int pid);

// Peso (turnos por ronda) segun la prioridad del proceso. Lo usan tanto la
// inicializacion/seteo de prioridad (process.c) como el scheduler (scheduler.c).
int weight_for(PCB *p);

#endif
