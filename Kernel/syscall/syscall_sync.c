


#include <syscall_dispatcher.h>
#include <semaphore.h>
#include <pipe.h>

uint64_t sys_sem_open(uint64_t name_ptr, uint64_t initial_value) {
    if (name_ptr == 0) {
        return (uint64_t)-1;
    }
    return (uint64_t)sem_open((const char *)name_ptr, initial_value);
}

uint64_t sys_sem_close(uint64_t name_ptr) {
    if (name_ptr == 0) {
        return (uint64_t)-1;
    }
    return (uint64_t)sem_close((const char *)name_ptr);
}

uint64_t sys_sem_wait(uint64_t name_ptr) {
    if (name_ptr == 0) {
        return (uint64_t)-1;
    }
    return (uint64_t)sem_wait((const char *)name_ptr);
}

uint64_t sys_sem_post(uint64_t name_ptr) {
    if (name_ptr == 0) {
        return (uint64_t)-1;
    }
    return (uint64_t)sem_post((const char *)name_ptr);
}

uint64_t sys_pipe_open(void) {
    return (uint64_t)pipe_create();
}

uint64_t sys_pipe_close(uint64_t pipe_id) {
    pipe_close((int)pipe_id);
    return 0;
}
