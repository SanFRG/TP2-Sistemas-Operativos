




#include <syscall_dispatcher.h>
#include <textConsole.h>
#include <keyboard.h>
#include <process.h>
#include <pipe.h>
#include <exceptions.h>



extern RegisterFrame user_snapshot;
extern uint8_t snapshot_ready;


uint64_t sys_clear(uint64_t color) {
    (void)color;
    tc_clear();
    return 0;
}


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


uint64_t sys_get_key(void) {
    if (hasNextKey()) {
        return (uint64_t)nextKey();
    }
    return 0;
}



uint64_t sys_get_regs(uint64_t buffer_ptr) {
    if (buffer_ptr == 0) {
        return -1;
    }

    RegisterFrame* buffer = (RegisterFrame*)buffer_ptr;

    if (snapshot_ready) {

        *buffer = user_snapshot;
        return 0;
    }


    return -1;
}
