#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

void sem_system_init(void);
int sem_open(const char *name, uint64_t initial_value);
int sem_close(const char *name);
int sem_wait(const char *name);
int sem_post(const char *name);
void sem_remove_waiter(int pid);




int sem_force_post_index(int sem_idx);

#endif
