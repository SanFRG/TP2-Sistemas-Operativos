#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

void sem_system_init(void);
int sem_open(const char *name, uint64_t initial_value);
int sem_close(const char *name);
int sem_wait(const char *name);
int sem_post(const char *name);
void sem_remove_waiter(int pid);

// Postea un semaforo por indice de tabla, SIN tocar el tracking de tokens.
// La usa process.c para devolver los tokens que un proceso tenia tomados al
// morir (process_release_held_sems), restaurando la invariante del semaforo.
int sem_force_post_index(int sem_idx);

#endif
