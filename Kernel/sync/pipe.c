#include <pipe.h>
#include <process.h>
#include <interrupts.h>
#include <lib.h>
#include <stdint.h>

typedef struct {
    uint8_t in_use;
    char buf[PIPE_BUFFER_SIZE];
    int read_pos;
    int write_pos;
    int count;
    int write_open;
    int read_open;
    int reader_pid;
    int writer_pid;
} pipe_t;

static pipe_t pipe_table[MAX_PIPES];

static uint64_t pipe_lock_acquire(void) {
    return _save_irq();
}

static void pipe_lock_release(uint64_t flags) {
    _restore_irq(flags);
}

void pipe_system_init(void) {
    memset(pipe_table, 0, sizeof(pipe_table));
}

int pipe_create(void) {
    uint64_t flags = pipe_lock_acquire();
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipe_table[i].in_use) {
            memset(&pipe_table[i], 0, sizeof(pipe_t));
            pipe_table[i].in_use = 1;
            pipe_table[i].write_open = 1;
            pipe_table[i].read_open = 1;
            pipe_table[i].reader_pid = -1;
            pipe_table[i].writer_pid = -1;
            pipe_lock_release(flags);
            return i + PIPE_FD_MIN;
        }
    }
    pipe_lock_release(flags);
    return -1;
}

int pipe_read(int pipe_id, char *buf, int max_len) {
    int idx = pipe_id - PIPE_FD_MIN;
    uint64_t flags;
    if (idx < 0 || idx >= MAX_PIPES || max_len <= 0 || buf == 0) return -1;

    flags = pipe_lock_acquire();
    pipe_t *p = &pipe_table[idx];

    while (p->in_use && p->count == 0 && p->write_open) {
        p->reader_pid = process_get_current_pid();
        if (process_block_current() != 0) {
            p->reader_pid = -1;
            pipe_lock_release(flags);
            return -1;
        }
        pipe_lock_release(flags);
        _yield();
        flags = pipe_lock_acquire();
        p = &pipe_table[idx];
    }

    if (!p->in_use) { pipe_lock_release(flags); return -1; }

    int n = 0;
    while (n < max_len && p->count > 0) {
        buf[n++] = p->buf[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
        p->count--;
    }

    if (n > 0 && p->writer_pid >= 0) {
        int wpid = p->writer_pid;
        p->writer_pid = -1;
        pipe_lock_release(flags);
        process_unblock(wpid);
        return n;
    }

    pipe_lock_release(flags);
    return n;
}

int pipe_write(int pipe_id, const char *buf, int len) {
    int idx = pipe_id - PIPE_FD_MIN;
    uint64_t flags;
    if (idx < 0 || idx >= MAX_PIPES || len <= 0 || buf == 0) return -1;

    flags = pipe_lock_acquire();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use || !p->read_open) { pipe_lock_release(flags); return -1; }

    int written = 0;
    while (written < len) {
        if (!p->in_use || !p->read_open) break;

        while (p->in_use && p->read_open && p->count == PIPE_BUFFER_SIZE) {
            p->writer_pid = process_get_current_pid();
            if (process_block_current() != 0) {
                p->writer_pid = -1;
                pipe_lock_release(flags);
                return written > 0 ? written : -1;
            }
            pipe_lock_release(flags);
            _yield();
            flags = pipe_lock_acquire();
            p = &pipe_table[idx];
        }

        if (!p->in_use || !p->read_open) break;

        while (written < len && p->count < PIPE_BUFFER_SIZE) {
            p->buf[p->write_pos] = buf[written++];
            p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
            p->count++;
        }

        if (p->reader_pid >= 0) {
            int rpid = p->reader_pid;
            p->reader_pid = -1;
            pipe_lock_release(flags);
            process_unblock(rpid);
            flags = pipe_lock_acquire();
            p = &pipe_table[idx];
        }
    }

    pipe_lock_release(flags);
    return written;
}

void pipe_close_write_end(int pipe_id) {
    int idx = pipe_id - PIPE_FD_MIN;
    uint64_t flags;
    if (idx < 0 || idx >= MAX_PIPES) return;

    flags = pipe_lock_acquire();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use) { pipe_lock_release(flags); return; }

    p->write_open = 0;

    if (p->reader_pid >= 0) {
        int rpid = p->reader_pid;
        p->reader_pid = -1;
        pipe_lock_release(flags);
        process_unblock(rpid);
        flags = pipe_lock_acquire();
        p = &pipe_table[idx];
    }

    if (!p->read_open && !p->write_open) {
        memset(p, 0, sizeof(*p));
    }
    pipe_lock_release(flags);
}

void pipe_close_read_end(int pipe_id) {
    int idx = pipe_id - PIPE_FD_MIN;
    uint64_t flags;
    if (idx < 0 || idx >= MAX_PIPES) return;

    flags = pipe_lock_acquire();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use) { pipe_lock_release(flags); return; }

    p->read_open = 0;

    if (p->writer_pid >= 0) {
        int wpid = p->writer_pid;
        p->writer_pid = -1;
        pipe_lock_release(flags);
        process_unblock(wpid);
        flags = pipe_lock_acquire();
        p = &pipe_table[idx];
    }

    if (!p->read_open && !p->write_open) {
        memset(p, 0, sizeof(*p));
    }
    pipe_lock_release(flags);
}

void pipe_close(int pipe_id) {
    pipe_close_read_end(pipe_id);
    pipe_close_write_end(pipe_id);
}

void pipe_on_process_exit(int fd0, int fd1, int fd2) {
    if (fd0 >= PIPE_FD_MIN) pipe_close_read_end(fd0);
    if (fd1 >= PIPE_FD_MIN) pipe_close_write_end(fd1);
    if (fd2 >= PIPE_FD_MIN) pipe_close_write_end(fd2);
}
