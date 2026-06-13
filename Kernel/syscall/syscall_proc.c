/* Syscalls de procesos: consultas (pid/ps), control (kill/block/unblock/nice),
 * espera (waitpid), cesion de CPU (yield), terminacion (exit), creacion (normal
 * y con pipes) y utilidades (check ctrl+c, loop_inc). La syscall_table
 * (syscall_dispatcher.c) las referencia por su declaracion en el header. */
#include <syscall_dispatcher.h>
#include <process.h>
#include <keyboard.h>
#include <interrupts.h>

uint64_t sys_getpid(void) {
    return (uint64_t)process_get_current_pid();
}

uint64_t sys_kill(uint64_t pid) {
    return (uint64_t)process_kill((int)pid);
}

uint64_t sys_block(uint64_t pid) {
    return (uint64_t)process_block((int)pid);
}

uint64_t sys_unblock(uint64_t pid) {
    return (uint64_t)process_unblock((int)pid);
}

uint64_t sys_nice(uint64_t pid, uint64_t new_priority) {
    return (uint64_t)process_set_priority((int)pid, (int)new_priority);
}

uint64_t sys_waitpid(uint64_t pid) {
    return (uint64_t)process_wait((int)pid);
}

uint64_t sys_ps(uint64_t buffer_ptr, uint64_t max_entries) {
    if (buffer_ptr == 0) {
        return (uint64_t)-1;
    }
    return (uint64_t)process_list((process_info *)buffer_ptr, max_entries);
}

uint64_t sys_yield(void) {
    process_exit_if_kill_pending();
    _yield();
    return 0;
}

// SYS_EXIT: termina el proceso actual. No retorna (process_exit cede el CPU).
uint64_t sys_exit(uint64_t exit_code) {
    process_exit((int)exit_code);
    return 0;  // formal: process_exit nunca devuelve el control aca
}

uint64_t sys_create_process(uint64_t name_ptr, uint64_t entry_ptr, uint64_t arg_ptr, uint64_t priority, uint64_t foreground) {
    if (name_ptr == 0 || entry_ptr == 0) {
        return (uint64_t)-1;
    }

    return (uint64_t)process_create((const char *)name_ptr,
                                    (void (*)(void *))entry_ptr,
                                    (void *)arg_ptr,
                                    (int)priority,
                                    (int)foreground);
}

uint64_t sys_check_ctrl_c(void) {
    return (uint64_t)get_and_clear_ctrl_c();
}

uint64_t sys_loop_inc(void) {
    return (uint64_t)process_loop_inc();
}

uint64_t sys_create_process_piped(uint64_t args_ptr) {
    if (args_ptr == 0) return (uint64_t)-1;
    struct {
        char *name;
        void (*entry)(void *);
        void *arg;
        int priority;
        int foreground;
        int fd_in;
        int fd_out;
    } *a = (void *)args_ptr;
    return (uint64_t)process_create_with_fds(a->name, a->entry, a->arg,
                                              a->priority, a->foreground,
                                              a->fd_in, a->fd_out);
}
