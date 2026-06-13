/* Syscalls de I/O por consola y teclado: limpiar/colorear la pantalla, escribir
 * (a consola o a un pipe si stdout esta redirigido), leer una tecla sin bloqueo
 * y obtener el snapshot de registros capturado al presionar ESC. La syscall_table
 * (syscall_dispatcher.c) las referencia por su declaracion en el header.
 * sys_read vive aparte en syscall_history.c por el historial de comandos. */
#include <syscall_dispatcher.h>
#include <textConsole.h>
#include <keyboard.h>
#include <process.h>
#include <pipe.h>
#include <exceptions.h>

// Variables externas desde interrupts.asm: snapshot de registros que se captura
// cuando el usuario presiona ESC, devuelto por sys_get_regs.
extern RegisterFrame user_snapshot;
extern uint8_t snapshot_ready;

// SYS_CLEAR: Clear screen
uint64_t sys_clear(uint64_t color) {
    (void)color;
    tc_clear();
    return 0;
}

// SYS_SET_COLOR: Set current text color (VGA attribute byte)
uint64_t sys_set_color(uint64_t attr) {
    tc_set_color((uint8_t)attr);
    return 0;
}

uint64_t sys_write(uint64_t fd, uint64_t str_ptr, uint64_t length) {
    if (fd <= 2) {
        int pipe_fd = process_get_current_fd((int)fd);
        if (pipe_fd >= PIPE_FD_MIN) {
            int n = pipe_write(pipe_fd, (const char *)str_ptr, (int)length);
            return n < 0 ? 0 : (uint64_t)n;
        }
    }
    tc_write((const char *)str_ptr, length);
    return length;
}

// SYS_GET_KEY: Get key non-blocking (returns scancode or 0 if no key)
uint64_t sys_get_key(void) {
    if (hasNextKey()) {
        return (uint64_t)nextKey();
    }
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
