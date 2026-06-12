/* sys_read: lectura de linea desde stdin (teclado) con historial navegable por
 * flechas, o lectura directa de un pipe si stdin esta redirigido. Separado de
 * syscall_dispatcher.c por tamano; la syscall_table lo referencia por su
 * declaracion en syscall_dispatcher.h. */
#include <syscall_dispatcher.h>
#include <keyboard.h>
#include <lib.h>
#include <interrupts.h>
#include <textConsole.h>
#include <process.h>
#include <pipe.h>

// ===== Historial de comandos (navegable con flechas en sys_read) =====
#define HISTORY_SIZE     16
#define HISTORY_LINE_MAX 256

static char history[HISTORY_SIZE][HISTORY_LINE_MAX];
static int history_count = 0;  // entradas almacenadas (tope HISTORY_SIZE)
static int history_head = 0;   // índice donde se escribirá la próxima (ring)

static int kstrlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

// Devuelve la entrada en posición lógica 0..history_count-1 (0 = más antigua)
static const char *history_get(int logical) {
    int idx = (history_head - history_count + logical + HISTORY_SIZE) % HISTORY_SIZE;
    return history[idx];
}

// Agrega una línea al historial (ignora vacías y duplicados de la más reciente)
static void history_add(const char *line) {
    if (line[0] == '\0') return;
    if (history_count > 0) {
        const char *last = history_get(history_count - 1);
        int same = 1;
        for (int k = 0; ; k++) {
            if (last[k] != line[k]) { same = 0; break; }
            if (line[k] == '\0') break;
        }
        if (same) return;
    }
    strncpy(history[history_head], line, HISTORY_LINE_MAX);
    history_head = (history_head + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

uint64_t sys_read(uint64_t buffer, uint64_t max_len) {
    char* buf = (char*)buffer;

    if (max_len == 0) return 0;

    /* if stdin is redirected to a pipe, read from it directly */
    int pipe_fd = process_get_current_fd(0);
    if (pipe_fd >= PIPE_FD_MIN) {
        int n = pipe_read(pipe_fd, buf, (int)(max_len > 1 ? max_len - 1 : max_len));
        if (n > 0 && (uint64_t)n < max_len) buf[n] = '\0';
        return (uint64_t)(n < 0 ? 0 : n);
    }

    uint64_t i = 0;
    // Cursor de navegación del historial: 0..history_count-1 selecciona una
    // entrada; history_count = línea nueva en edición.
    int nav = history_count;

    while (i + 1 < max_len) {  // Reserve space for null terminator
        // Check for Ctrl+C (interrupt current read)
        if (get_and_clear_ctrl_c()) {
            return (uint64_t)-1;  // Signal interrupt
        }

        // Check for Ctrl+D (EOF signal)
        if (get_and_clear_ctrl_d()) {
            buf[i] = '\0';
            return i;  // Return what we have (may be 0 = EOF)
        }

        // Wait for a key
        while (!hasNextKey()) {
            // Re-check Ctrl+C/D while waiting
            if (get_and_clear_ctrl_c()) {
                return (uint64_t)-1;
            }
            if (get_and_clear_ctrl_d()) {
                buf[i] = '\0';
                return i;
            }
            _hlt();  // Halt until next interrupt
        }

        int scancode = nextKey();

        char c = scancode_to_char(scancode);

        if (c == 0) {
            continue;  // Ignore non-character scancodes (Shift, Ctrl, release codes, etc.)
        }

        if (c == '\n') {
            // Enter pressed: echo newline, save to history and return
            tc_write("\n", 1);
            buf[i] = '\0';
            history_add(buf);
            return i;
        } else if (c == KEY_UP || c == KEY_DOWN) {
            // Navegación del historial: calcular la nueva posición del cursor
            if (c == KEY_UP) {
                if (nav > 0) nav--;
                else continue;  // ya en la más antigua
            } else {  // KEY_DOWN
                if (nav < history_count) nav++;
                else continue;  // ya en la línea nueva
            }

            // Borrar la línea actual en pantalla (backspace ya limpia la celda)
            while (i > 0) {
                tc_write("\b", 1);
                i--;
            }

            // Cargar la entrada seleccionada (o línea vacía si nav == count)
            if (nav < history_count) {
                const char *entry = history_get(nav);
                int len = kstrlen(entry);
                if (len > (int)(max_len - 1)) len = (int)(max_len - 1);
                memcpy(buf, entry, len);
                i = (uint64_t)len;
                tc_write(buf, i);
            }
            // si nav == history_count, la línea queda vacía (i == 0)
        } else if (c == '\b') {
            // Backspace: remove last character from buffer and echo backspace
            if (i > 0) {
                i--;
                tc_write("\b", 1);
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
                tc_write(" ", 1);
            }
        } else if (c >= 32 && c < 127) {
            // Printable character: add to buffer and echo
            buf[i++] = c;
            tc_write(&c, 1);
        }
        // Ignore other characters (control characters, etc.)
    }

    // Buffer full without Enter pressed
    buf[i] = '\0';
    return i;
}
