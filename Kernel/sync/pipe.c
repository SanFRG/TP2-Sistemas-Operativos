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
    int reader_pid;   /* PID blocked waiting to read, -1 if none */
    int writer_pid;   /* PID blocked waiting to write, -1 if none */
} pipe_t;

static pipe_t pipe_table[MAX_PIPES];

void pipe_system_init(void) {
    memset(pipe_table, 0, sizeof(pipe_table));
}

int pipe_create(void) {
    _cli();
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipe_table[i].in_use) {
            memset(&pipe_table[i], 0, sizeof(pipe_t));
            pipe_table[i].in_use = 1;
            pipe_table[i].write_open = 1;
            pipe_table[i].read_open = 1;
            pipe_table[i].reader_pid = -1;
            pipe_table[i].writer_pid = -1;
            _sti();
            return i + PIPE_FD_MIN;
        }
    }
    _sti();
    return -1;
}

int pipe_read(int pipe_id, char *buf, int max_len) {
    int idx = pipe_id - PIPE_FD_MIN;
    if (idx < 0 || idx >= MAX_PIPES || max_len <= 0 || buf == 0) return -1;

    _cli();
    pipe_t *p = &pipe_table[idx];

    /* block while buffer is empty and write end is still open */
    while (p->in_use && p->count == 0 && p->write_open) {
        p->reader_pid = process_get_current_pid();
        if (process_block_current() != 0) {
            p->reader_pid = -1;
            _sti();
            return -1;
        }
        _sti(); 
        _yield();//se hace porque el proceso se bloquea y no va a ser elegido por el scheduler hasta que lo desbloqueemos, entonces cede el CPU para que otro proceso corra (posiblemente el escritor que necesitamos que escriba algo para desbloquearnos)
        _cli();
        p = &pipe_table[idx];
    }

    if (!p->in_use) { _sti(); return -1; }

    /* drain up to max_len bytes from circular buffer */
    int n = 0;
    while (n < max_len && p->count > 0) {
        buf[n++] = p->buf[p->read_pos];
        p->read_pos = (p->read_pos + 1) % PIPE_BUFFER_SIZE;
        p->count--;
    }

    /* wake a blocked writer now that there is space */
    if (n > 0 && p->writer_pid >= 0) {
        int wpid = p->writer_pid;
        p->writer_pid = -1;
        _sti();
        process_unblock(wpid);
        return n;
    }

    _sti();
    return n;  /* 0 means EOF (count==0 and write end closed) */
}

int pipe_write(int pipe_id, const char *buf, int len) {
    int idx = pipe_id - PIPE_FD_MIN;
    if (idx < 0 || idx >= MAX_PIPES || len <= 0 || buf == 0) return -1;

    _cli();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use || !p->read_open) { _sti(); return -1; }

    int written = 0;
    while (written < len) {
        if (!p->in_use || !p->read_open) break;

        /* block while buffer is full */
        while (p->in_use && p->read_open && p->count == PIPE_BUFFER_SIZE) {
            p->writer_pid = process_get_current_pid();
            if (process_block_current() != 0) {
                p->writer_pid = -1;
                _sti();
                return written > 0 ? written : -1;
            }
            _sti();
            _yield();
            _cli();
            p = &pipe_table[idx];
        }

        if (!p->in_use || !p->read_open) break;

        /* fill as many bytes as fit */
        while (written < len && p->count < PIPE_BUFFER_SIZE) {
            p->buf[p->write_pos] = buf[written++];
            p->write_pos = (p->write_pos + 1) % PIPE_BUFFER_SIZE;
            p->count++;
        }

        /* wake a blocked reader */
        if (p->reader_pid >= 0) {
            int rpid = p->reader_pid;
            p->reader_pid = -1;
            _sti();
            process_unblock(rpid);
            _cli();
            p = &pipe_table[idx];
        }
    }

    _sti();
    return written;
}

void pipe_close_write_end(int pipe_id) {
    int idx = pipe_id - PIPE_FD_MIN;
    if (idx < 0 || idx >= MAX_PIPES) return;

    _cli();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use) { _sti(); return; }

    p->write_open = 0;

    /* unblock reader so it can observe EOF */
    if (p->reader_pid >= 0) {
        int rpid = p->reader_pid;
        p->reader_pid = -1;
        _sti();
        process_unblock(rpid);
        _cli();
        p = &pipe_table[idx];
    }

    if (!p->read_open && !p->write_open) {
        memset(p, 0, sizeof(*p));
    }
    _sti();
}

void pipe_close_read_end(int pipe_id) {
    int idx = pipe_id - PIPE_FD_MIN;
    if (idx < 0 || idx >= MAX_PIPES) return;

    _cli();
    pipe_t *p = &pipe_table[idx];
    if (!p->in_use) { _sti(); return; }

    p->read_open = 0;

    /* unblock writer so it sees broken pipe */
    if (p->writer_pid >= 0) {
        int wpid = p->writer_pid;
        p->writer_pid = -1;
        _sti();
        process_unblock(wpid);
        _cli();
        p = &pipe_table[idx];
    }

    if (!p->read_open && !p->write_open) {
        memset(p, 0, sizeof(*p));
    }
    _sti();
}

/* Cierra ambos extremos del pipe a la fuerza y libera el slot.
 * Se usa cuando la shell aborta un pipe a medio armar: desbloquea a
 * cualquier proceso que haya quedado esperando y recupera el recurso. */
void pipe_close(int pipe_id) {
    pipe_close_read_end(pipe_id);
    pipe_close_write_end(pipe_id);
}

void pipe_on_process_exit(int fd0, int fd1, int fd2) {
    if (fd0 >= PIPE_FD_MIN) pipe_close_read_end(fd0);
    if (fd1 >= PIPE_FD_MIN) pipe_close_write_end(fd1);
    if (fd2 >= PIPE_FD_MIN) pipe_close_write_end(fd2);
}
