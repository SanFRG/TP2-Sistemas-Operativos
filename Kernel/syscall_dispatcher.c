/* Minimal syscall dispatcher for testing */
#include <naiveConsole.h>
#include <syscall_dispatcher.h>
#include <keyboard.h>
#include <time.h>
#include <videoDriver.h>
#include <lib.h>
#include <time.h>
#include <interrupts.h>
#include <reg_printer.h>
#include <exceptions.h>

// Variables externas desde interrupts.asm
extern RegisterFrame user_snapshot;
extern uint8_t snapshot_ready;

// Console cursor state (maintained by kernel)
static uint64_t console_x = 10;
static uint64_t console_y = 10;
static int console_scale = 1;
static const uint32_t console_color = 0x00FFFFFF;
static const uint32_t console_bg = 0x00000000;
static const int char_width = 8;
static const int char_height = 15;

// Limits for scale
#define MIN_SCALE 1
#define MAX_SCALE 5
static void checkAndScroll(void);

// Tabla de syscalls (puntero exportado para acceso desde int 0x80)
// Cada entrada es un puntero a función
void* syscall_table[SYS_COUNT] = {
    &sys_read,           // 0: SYS_READ
    &sys_write,          // 1: SYS_WRITE
    &sys_clear,          // 2: SYS_CLEAR
    &sys_draw_at,        // 3: SYS_DRAW_AT
    &sys_time,           // 4: SYS_TIME
    &sys_ticks,          // 5: SYS_TICKS
    &sys_set_scale,      // 6: SYS_SET_SCALE
    &sys_draw_rect,      // 7: SYS_DRAW_RECT
    &sys_get_key,        // 8: SYS_GET_KEY
    &sys_get_screen_info,// 9: SYS_GET_SCREEN_INFO
    &sys_sleep,          // 10: SYS_SLEEP
    &sys_speaker_play,   // 11: SYS_SPEAKER_PLAY
    &sys_speaker_stop,   // 12: SYS_SPEAKER_STOP
    &sys_get_regs        // 13: SYS_GET_REGS
};

// ========== SYSCALL HANDLERS ==========

// SYS_CLEAR: Clear screen
uint64_t sys_clear(uint64_t color) {
    uint32_t bg_color = color ? (uint32_t)color : 0x00000000;
    clearScreen(bg_color);
    
    // Reset console cursor to top-left
    console_x = 10;
    console_y = 10;
    
    return 0;
}
uint64_t sys_ticks(void) {
    return getTicks();
}

// SYS_TIME: Get current time/date based on ticks (NOT RTC)--> empaqueta el tiempo para devolverla a userland   
uint64_t sys_time() {
    Time t = getTime();
    // empaqueta [YY|MM|DD|hh|mm|ss]
    return ((uint64_t)t.year   << 40) |
           ((uint64_t)t.month  << 32) |
           ((uint64_t)t.day    << 24) |
           ((uint64_t)t.hours  << 16) |
           ((uint64_t)t.minutes<<  8) |
           (uint64_t)t.seconds;
}

// SYS_SET_SCALE: Set text scale (zoom)
uint64_t sys_set_scale(uint64_t delta) {
    int d = (int)delta;
    // If delta == -2, just return current scale without changing
    if (d == -2) {
        return console_scale;
    }
    
    int new_scale = console_scale + d;
    
    if (new_scale < MIN_SCALE || new_scale > MAX_SCALE) {
        return -1;  // Error: fuera de rango
    }
    console_scale = new_scale;
    return 0;
}

// SYS_DRAW_AT: Draw text at specific coordinates (for UI/games)
uint64_t sys_draw_at(uint64_t str_ptr, uint64_t length, uint64_t x, uint64_t y, uint64_t color) {
    char* str = (char*)str_ptr;
    int len = (int)length;
    uint32_t text_color = color ? (uint32_t)color : console_color;  // Use provided color or default
    
    if (!str || len <= 0) {
        return 0;
    }
    
    for (int i = 0; i < len && str[i] != 0; i++) {
        if (str[i] >= 32 && str[i] < 127) {
            drawChar(str[i], x + (i * char_width * console_scale), y, text_color, console_scale);
        }
    }
    return len;
}

// Helper function to check and handle scrolling
static void checkAndScroll(void) {
    if (console_y > getScreenHeight() - (char_height * console_scale)) {
        scrollUpLines(1, console_scale, console_bg);
        console_y -= char_height * console_scale;
    }
}

uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length) {
    // fd = file descriptor (1=stdout, 2=stderr - currently ignored)
    // str_ptr = string pointer
    // length = length
    // Returns: number of bytes written

    char* str = (char*)str_ptr;
    int len = (int)length;
    
    for (int i = 0; i < len && str[i] != 0; i++) {
        char c = str[i];
        
        if (c == '\n') {
            // Newline: reset x and advance y
            console_x = 10;
            console_y += char_height * console_scale;
            checkAndScroll();
        } else if (c == '\b') {
            // Backspace: move cursor back and erase character
            if (console_x > 10) {
                console_x -= char_width * console_scale;
                drawChar(127, console_x, console_y, console_bg, console_scale);
            } 
            else if (console_y > 10) {
                console_y -= char_height * console_scale;
                // Calcular cuántos caracteres caben en una línea
               // int max_x = getScreenWidth() - 10;  // Espacio disponible desde margen
                int chars_per_line = (getScreenWidth() - 10) / (char_width * console_scale);
                // Posicionar al final de la línea anterior (último carácter)
                console_x = 10 + ((chars_per_line - 1) * char_width * console_scale);
                drawChar(127, console_x, console_y, console_bg, console_scale);
            }
        } else if (c >= 32 && c < 127) {
            // Printable character
            drawChar(c, console_x, console_y, console_color, console_scale);
            console_x += char_width * console_scale;
            
            // Horizontal wrap
            if (console_x > getScreenWidth() - (char_width * console_scale)) {
                console_x = 10;
                console_y += char_height * console_scale;
                checkAndScroll();
            }
        }
    }
    return len;
}

uint64_t sys_read(uint64_t buffer, uint64_t max_len) {
    char* buf = (char*)buffer;
    int i = 0;
    
    if (max_len == 0) {
        return 0;
    }
    
    while (i < max_len - 1) {  // Reserve space for null terminator
        // Wait for a key
        while (!hasNextKey()) {
            _hlt();  // Halt until next interrupt
        }
        
        int scancode = nextKey();
        
        // Check for ESC key (scancode 0x01)
        // if (scancode == 0x01) {
        //     save_regs_on_esc();  // Save registers silently
        //     continue;  // Don't add to buffer, just continue reading
        // }
        
        char c = scancode_to_char(scancode);
        
        if (c == 0) {
            continue;  // Ignore non-character scancodes (Shift, release codes, etc.)
        }
        
        if (c == '\n') {
            // Enter pressed: echo newline and return
            sys_write(1, (uint64_t)"\n", 1);
            buf[i] = '\0';
            return i;
        } else if (c == '\b') {
            // Backspace: remove last character from buffer and echo backspace
            if (i > 0) {
                i--;
                sys_write(1, (uint64_t)"\b", 1);
            }
        } else if (c == '\t') {
            // Tab: expand to spaces (tab stop = 4)
            // Calculate current column (relative to margin)
            int col = i;  // Simplified: use buffer position as column
            int next_stop = ((col / 4) + 1) * 4;
            int spaces = next_stop - col;
            
            // Add spaces to buffer and echo them
            for (int j = 0; j < spaces && i < max_len - 1; j++) {
                buf[i++] = ' ';
                sys_write(1, (uint64_t)" ", 1);
            }
        } else if (c >= 32 && c < 127) {
            // Printable character: add to buffer and echo
            buf[i++] = c;
            sys_write(1, (uint64_t)&c, 1);
        }
        // Ignore other characters (control characters, etc.)
    }
    
    // Buffer full without Enter pressed
    buf[i] = '\0';
    return i;
}

// SYS_DRAW_RECT: Draw filled rectangle at specific coordinates
uint64_t sys_draw_rect(uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t color) {
    fillRect(x, y, width, height, (uint32_t)color);
    return 0;
}

// SYS_GET_KEY: Get key non-blocking (returns scancode or 0 if no key)
uint64_t sys_get_key(void) {
    if (hasNextKey()) {
        return (uint64_t)nextKey();
    }
    return 0;
}

// SYS_GET_SCREEN_INFO: Get screen resolution
// Returns width in upper 32 bits, height in lower 32 bits
uint64_t sys_get_screen_info(void) {
    uint16_t width = getScreenWidth();
    uint16_t height = getScreenHeight();
    
    // Retornar width en los 32 bits superiores, height en los inferiores
    return ((uint64_t)width << 32) | (uint64_t)height;
}

// SYS_SLEEP: Sleep for specified number of ticks
uint64_t sys_sleep(uint64_t ticks) {
    sleep((int)ticks);
    return 0;
}

// SYS_SPEAKER_PLAY: Play sound on PC speaker at given frequency (Hz)
uint64_t sys_speaker_play(uint64_t frequency) {
    if (frequency == 0 || frequency > 20000) {
        return -1;  // Invalid frequency
    }
    
    uint32_t divisor = 1193180 / frequency;
    
    // Set PIT to the desired frequency
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(divisor));
    outb(0x42, (uint8_t)(divisor >> 8));
    
    // Enable PC speaker
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
    
    return 0;
}

// SYS_SPEAKER_STOP: Stop PC speaker sound
uint64_t sys_speaker_stop(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
    return 0;
}

// SYS_GET_REGS: Get CPU registers snapshot
// Returns the registers saved when ESC was pressed
uint64_t sys_get_regs(uint64_t buffer_ptr) {
    if (buffer_ptr == 0) {
        return -1;  // Invalid buffer
    }
    
    RegisterFrame* buffer = (RegisterFrame*)buffer_ptr;
    
    if (snapshot_ready) {
        // Si se presionó ESC, retornar el snapshot capturado
        *buffer = user_snapshot;
        return 0;
    }
    
    // Si no se presionó ESC, no hay snapshot disponible
    return -1;
}
