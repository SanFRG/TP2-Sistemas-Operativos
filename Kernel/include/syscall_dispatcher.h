#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>
#include <exceptions.h>

// Syscall numbers
enum {
    SYS_READ = 0,      // Read from stdin (keyboard)
    SYS_WRITE = 1,     // Write to stdout (screen) - auto cursor
    SYS_CLEAR = 2,     // Clear screen
    SYS_DRAW_AT = 3,   // Draw text at specific coordinates
    SYS_TIME = 4,      // Get current time/date
    SYS_TICKS = 5,     // Get system ticks
    SYS_SET_SCALE = 6, // Set text scale (zoom)
    SYS_DRAW_RECT = 7, // Draw filled rectangle
    SYS_GET_KEY = 8,   // Get key non-blocking (returns 0 if no key)
    SYS_GET_SCREEN_INFO = 9,  // Get screen resolution (returns width in upper 32 bits, height in lower 32 bits)
    SYS_SLEEP = 10,    // Sleep for specified ticks
    SYS_SPEAKER_PLAY = 11,  // Play sound on PC speaker (frequency in Hz)
    SYS_SPEAKER_STOP = 12,  // Stop PC speaker sound
    SYS_GET_REGS = 13,      // Get current CPU registers snapshot
    SYS_COUNT = 14
};

// Tabla de syscalls (array de punteros, accedida desde int 0x80)
extern void* syscall_table[];

// Syscall handlers
uint64_t sys_read(uint64_t buffer, uint64_t max_len);
uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length);
uint64_t sys_clear(uint64_t color);
uint64_t sys_time();
uint64_t sys_set_scale(uint64_t delta);
uint64_t sys_draw_at(uint64_t str_ptr, uint64_t length, uint64_t x, uint64_t y, uint64_t color);
uint64_t sys_ticks(void);
uint64_t sys_draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t color);
uint64_t sys_get_key(void);
uint64_t sys_get_screen_info(void);
uint64_t sys_sleep(uint64_t ticks);
uint64_t sys_speaker_play(uint64_t frequency);
uint64_t sys_speaker_stop(void);
uint64_t sys_get_regs(uint64_t buffer_ptr);

#endif
