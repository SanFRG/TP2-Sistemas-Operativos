#ifndef LIB_USER_H
#define LIB_USER_H

#include <stdint.h>

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_DRAW_AT = 3,
    SYS_TIME = 4,
    SYS_TICKS = 5,
    SYS_SET_SCALE = 6,
    SYS_DRAW_RECT = 7,
    SYS_GET_KEY = 8,
    SYS_GET_SCREEN_INFO = 9,
    SYS_SLEEP = 10,
    SYS_SPEAKER_PLAY = 11,
    SYS_SPEAKER_STOP = 12,
    SYS_GET_REGS = 13,
    SYS_COUNT = 14
};

// Register snapshot structure (must match kernel ExceptionFrame)
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} RegisterSnapshot;

// Syscall wrappers (implemented in asm/libasm.asm)
extern uint64_t read(char* buffer, int max_len);
extern uint64_t write(int fd, const char* str, int len);
extern uint64_t get_time();
extern uint64_t clear_screen(uint32_t color);
extern uint64_t set_scale(int delta);  // delta: 1 = aumentar, -1 = disminuir
extern uint64_t draw_at(const char* str, int len, int x, int y, uint32_t color);
extern uint64_t get_ticks(void);
extern uint64_t draw_rect(int x, int y, int width, int height, uint32_t color);
extern int get_key(void);  // Returns scancode or 0 if no key pressed
extern uint64_t get_screen_info(void);  // Returns (width << 32) | height
extern void sleep_ticks(int ticks);  // Sleep for specified number of ticks
extern uint64_t speaker_play(uint32_t frequency);  // Play sound on PC speaker (Hz)
extern uint64_t speaker_stop(void);  // Stop PC speaker sound
extern uint64_t get_regs(RegisterSnapshot* buffer);  // Get CPU registers snapshot

extern void trigger_invalid_opcode(void);  // Trigger Invalid Opcode exception (for testing)

// String functions
int strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strcasecmp(const char* s1, const char* s2);  // Case-insensitive compare

// Print helpers
void print(const char* str);
void println(const char* str);
void printInt(int num);
void printHex(uint64_t num);
void print2Digits(int num);

// Number conversion
void itoa(int num, char* buffer);
void hexToString(uint64_t num, char* buffer);

#endif
