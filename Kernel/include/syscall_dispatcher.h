#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <exceptions.h>
#include <memoryManager.h>

// Syscall numbers
enum {
    SYS_READ = 0,      // Read from stdin (keyboard)
    SYS_WRITE = 1,     // Write to stdout (screen) - auto cursor
    SYS_CLEAR = 2,     // Clear screen
    SYS_TIME = 3,      // Get current time/date
    SYS_TICKS = 4,     // Get system ticks
    SYS_GET_KEY = 5,   // Get key non-blocking (returns 0 if no key)
    SYS_SLEEP = 6,     // Sleep for specified ticks
    SYS_SPEAKER_PLAY = 7,  // Play sound on PC speaker (frequency in Hz)
    SYS_SPEAKER_STOP = 8,  // Stop PC speaker sound
    SYS_GET_REGS = 9,      // Get current CPU registers snapshot
    SYS_MEM_ALLOC = 10,    // Allocate memory
    SYS_MEM_FREE = 11,     // Free memory
    SYS_MEM_STATUS = 12,   // Query allocator status
    SYS_GETPID = 13,       // Get current PID
    SYS_KILL = 14,         // Kill process by PID
    SYS_BLOCK = 15,        // Block process by PID
    SYS_UNBLOCK = 16,      // Unblock process by PID
    SYS_NICE = 17,         // Change process priority
    SYS_WAITPID = 18,      // Wait for process termination
    SYS_PS = 19,           // List process table
    SYS_YIELD = 20,        // Yield CPU voluntarily
    SYS_CREATE_PROCESS = 21, // Create process
    SYS_EXIT = 22,           // Terminate current process
    SYS_CHECK_CTRL_C = 23,   // Check and clear Ctrl+C flag
    SYS_LOOP_INC = 24,        // Increment loop counter for current process
    SYS_SEM_OPEN = 25,       // Open/create named semaphore
    SYS_SEM_CLOSE = 26,      // Close named semaphore
    SYS_SEM_WAIT = 27,       // Wait/down on semaphore
    SYS_SEM_POST = 28,       // Post/up semaphore
    SYS_PIPE_OPEN = 29,      // Create pipe; returns pipe_id
    SYS_CREATE_PROCESS_PIPED = 30, // Create process with custom fd_in/fd_out
    SYS_PIPE_CLOSE = 31,     // Close both ends of a pipe and free the slot
    SYS_SET_COLOR = 32,      // Set current text color (VGA attribute byte)
    SYS_PIPE_OPEN_NAMED = 33, // Open/create named pipe
    SYS_SET_FD = 34,          // Set current process fd entry
    SYS_COUNT = 35
};

// Tabla de syscalls (array de punteros, accedida desde int 0x80)
extern void* syscall_table[];

// Syscall handlers
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
uint64_t sys_pipe_open_named(uint64_t name_ptr);
uint64_t sys_set_fd(uint64_t fd_index, uint64_t fd_value);

#endif
