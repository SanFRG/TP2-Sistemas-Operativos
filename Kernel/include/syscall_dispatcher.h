#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <exceptions.h>
#include <memoryManager.h>


enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_TIME = 3,
    SYS_TICKS = 4,
    SYS_GET_KEY = 5,
    SYS_SLEEP = 6,
    SYS_SPEAKER_PLAY = 7,
    SYS_SPEAKER_STOP = 8,
    SYS_GET_REGS = 9,
    SYS_MEM_ALLOC = 10,
    SYS_MEM_FREE = 11,
    SYS_MEM_STATUS = 12,
    SYS_GETPID = 13,
    SYS_KILL = 14,
    SYS_BLOCK = 15,
    SYS_UNBLOCK = 16,
    SYS_NICE = 17,
    SYS_WAITPID = 18,
    SYS_PS = 19,
    SYS_YIELD = 20,
    SYS_CREATE_PROCESS = 21,
    SYS_EXIT = 22,
    SYS_CHECK_CTRL_C = 23,
    SYS_LOOP_INC = 24,
    SYS_SEM_OPEN = 25,
    SYS_SEM_CLOSE = 26,
    SYS_SEM_WAIT = 27,
    SYS_SEM_POST = 28,
    SYS_PIPE_OPEN = 29,
    SYS_CREATE_PROCESS_PIPED = 30,
    SYS_PIPE_CLOSE = 31,
    SYS_SET_COLOR = 32,
    SYS_COUNT = 33
};


extern void* syscall_table[];


uint64_t sys_read(uint64_t buffer, uint64_t max_len);
uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length);
uint64_t sys_clear(uint64_t color);
uint64_t sys_time(void);
uint64_t sys_ticks(void);
uint64_t sys_get_key(void);
uint64_t sys_sleep(uint64_t ticks);
uint64_t sys_speaker_play(uint64_t frequency);
uint64_t sys_speaker_stop(void);
uint64_t sys_get_regs(uint64_t buffer_ptr);
uint64_t sys_mem_alloc(uint64_t size);
uint64_t sys_mem_free(uint64_t ptr);
uint64_t sys_mem_status(uint64_t status_ptr);
uint64_t sys_getpid(void);
uint64_t sys_kill(uint64_t pid);
uint64_t sys_block(uint64_t pid);
uint64_t sys_unblock(uint64_t pid);
uint64_t sys_nice(uint64_t pid, uint64_t new_priority);
uint64_t sys_waitpid(uint64_t pid);
uint64_t sys_ps(uint64_t buffer_ptr, uint64_t max_entries);
uint64_t sys_yield(void);
uint64_t sys_create_process(uint64_t name_ptr, uint64_t entry_ptr, uint64_t arg_ptr, uint64_t priority, uint64_t foreground);
uint64_t sys_exit(uint64_t exit_code);
uint64_t sys_check_ctrl_c(void);
uint64_t sys_loop_inc(void);
uint64_t sys_sem_open(uint64_t name_ptr, uint64_t initial_value);
uint64_t sys_sem_close(uint64_t name_ptr);
uint64_t sys_sem_wait(uint64_t name_ptr);
uint64_t sys_sem_post(uint64_t name_ptr);
uint64_t sys_pipe_open(void);
uint64_t sys_create_process_piped(uint64_t args_ptr);
uint64_t sys_pipe_close(uint64_t pipe_id);
uint64_t sys_set_color(uint64_t attr);

#endif
