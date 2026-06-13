












#include <syscall_dispatcher.h>



void* syscall_table[SYS_COUNT] = {
    &sys_read,
    &sys_write,
    &sys_clear,
    &sys_time,
    &sys_ticks,
    &sys_get_key,
    &sys_sleep,
    &sys_speaker_play,
    &sys_speaker_stop,
    &sys_get_regs,
    &sys_mem_alloc,
    &sys_mem_free,
    &sys_mem_status,
    &sys_getpid,
    &sys_kill,
    &sys_block,
    &sys_unblock,
    &sys_nice,
    &sys_waitpid,
    &sys_ps,
    &sys_yield,
    &sys_create_process,
    &sys_exit,
    &sys_check_ctrl_c,
    &sys_loop_inc,
    &sys_sem_open,
    &sys_sem_close,
    &sys_sem_wait,
    &sys_sem_post,
    &sys_pipe_open,
    &sys_create_process_piped,
    &sys_pipe_close,
    &sys_set_color
};
