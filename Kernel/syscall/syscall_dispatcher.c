/* Tabla de syscalls: mapea cada numero de syscall a su handler. La accede el
 * stub de int 0x80 (interrupts.asm) indexando por el numero en rax.
 *
 * Los handlers viven en archivos por dominio, todos declarados en
 * syscall_dispatcher.h (por eso la tabla puede referenciarlos por simbolo):
 *   - syscall_io.c      consola y teclado (clear, color, write, get_key, get_regs)
 *   - syscall_history.c lectura de linea con historial (read)
 *   - syscall_time.c    tiempo (time, ticks, sleep)
 *   - syscall_sound.c   PC speaker (play, stop)
 *   - syscall_mem.c     memory manager (alloc, free, status)
 *   - syscall_proc.c    procesos (pid, kill, block, nice, wait, ps, yield, exit, create...)
 *   - syscall_sync.c    semaforos y pipes
 */
#include <syscall_dispatcher.h>

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
    &sys_create_process, // 21: SYS_CREATE_PROCESS
    &sys_exit,           // 22: SYS_EXIT
    &sys_check_ctrl_c,   // 23: SYS_CHECK_CTRL_C
    &sys_loop_inc,       // 24: SYS_LOOP_INC
    &sys_sem_open,             // 25: SYS_SEM_OPEN
    &sys_sem_close,            // 26: SYS_SEM_CLOSE
    &sys_sem_wait,             // 27: SYS_SEM_WAIT
    &sys_sem_post,             // 28: SYS_SEM_POST
    &sys_pipe_open,            // 29: SYS_PIPE_OPEN
    &sys_create_process_piped, // 30: SYS_CREATE_PROCESS_PIPED
    &sys_pipe_close,           // 31: SYS_PIPE_CLOSE
    &sys_set_color             // 32: SYS_SET_COLOR
};
