#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <exceptions.h>

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
    SYS_COUNT = 10
};

// Tabla de syscalls (array de punteros, accedida desde int 0x80)
extern void* syscall_table[];

// Syscall handlers
uint64_t sys_read(uint64_t buffer, uint64_t max_len);
uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length);
uint64_t sys_clear(uint64_t color);
uint64_t sys_time();
uint64_t sys_ticks(void);
uint64_t sys_get_key(void);
uint64_t sys_sleep(uint64_t ticks);
uint64_t sys_speaker_play(uint64_t frequency);
uint64_t sys_speaker_stop(void);
uint64_t sys_get_regs(uint64_t buffer_ptr);

#endif
