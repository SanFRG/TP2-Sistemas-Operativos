/* Minimal syscall dispatcher for testing */
#include <naiveConsole.h>
#include <syscall_dispatcher.h>
#include <keyboard.h>
#include <time.h>
#include <lib.h>
#include <time.h>
#include <interrupts.h>
#include <reg_printer.h>
#include <exceptions.h>
#include <textConsole.h>
#include <memoryManager.h>
#include <process.h>

// Variables externas desde interrupts.asm
extern RegisterFrame user_snapshot;
extern uint8_t snapshot_ready;

// Tabla de syscalls (puntero exportado para acceso desde int 0x80)
// Cada entrada es un puntero a función
void* syscall_table[SYS_COUNT] = {
    &sys_read,           // 0: SYS_READ
    &sys_write,          // 1: SYS_WRITE
    &sys_clear,          // 2: SYS_CLEAR
    &sys_time,           // 3: SYS_TIME
    &sys_ticks,          // 4: SYS_TICKS
    &sys_get_key,        // 5: SYS_GET_KEY
    &sys_sleep,          // 6: SYS_SLEEP
    &sys_speaker_play,   // 7: SYS_SPEAKER_PLAY
    &sys_speaker_stop,   // 8: SYS_SPEAKER_STOP
    &sys_get_regs,       // 9: SYS_GET_REGS
    &sys_mem_alloc,      // 10: SYS_MEM_ALLOC
    &sys_mem_free,       // 11: SYS_MEM_FREE
    &sys_mem_status,     // 12: SYS_MEM_STATUS
    &sys_getpid,         // 13: SYS_GETPID
    &sys_kill,           // 14: SYS_KILL
    &sys_block,          // 15: SYS_BLOCK
    &sys_unblock,        // 16: SYS_UNBLOCK
    &sys_nice,           // 17: SYS_NICE
    &sys_waitpid,        // 18: SYS_WAITPID
    &sys_ps,             // 19: SYS_PS
    &sys_yield,          // 20: SYS_YIELD
    &sys_create_process  // 21: SYS_CREATE_PROCESS
};

// ========== SYSCALL HANDLERS ==========

// SYS_CLEAR: Clear screen
uint64_t sys_clear(uint64_t color) {
    (void)color;
    tc_clear();
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

uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length) {
    // fd = file descriptor (1=stdout, 2=stderr - currently ignored)
    // str_ptr = string pointer
    // length = length
    // Returns: number of bytes written

    (void)fd;
    tc_write((const char *)str_ptr, length);
    return length;
}

uint64_t sys_read(uint64_t buffer, uint64_t max_len) {
    char* buf = (char*)buffer;
    uint64_t i = 0;

    if (max_len == 0) {
        return 0;
    }

    while (i + 1 < max_len) {  // Reserve space for null terminator
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
            int col = (int)i;  // Simplified: use buffer position as column
            int next_stop = ((col / 4) + 1) * 4;
            int spaces = next_stop - col;

            // Add spaces to buffer and echo them
            for (int j = 0; j < spaces && i + 1 < max_len; j++) {
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

// SYS_GET_KEY: Get key non-blocking (returns scancode or 0 if no key)
uint64_t sys_get_key(void) {
    if (hasNextKey()) {
        return (uint64_t)nextKey();
    }
    return 0;
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

uint64_t sys_mem_alloc(uint64_t size) {
    return (uint64_t)mm_alloc(size);
}

uint64_t sys_mem_free(uint64_t ptr) {
    mm_free((void *)ptr);
    return 0;
}

uint64_t sys_mem_status(uint64_t status_ptr) {
    if (status_ptr == 0) {
        return (uint64_t)-1;
    }

    mm_get_status((mm_status_t *)status_ptr);
    return 0;
}

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
    _yield();
    return 0;
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
