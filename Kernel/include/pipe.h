#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUFFER_SIZE 256
#define MAX_PIPES 8
#define PIPE_FD_MIN 3

void pipe_system_init(void);
int  pipe_create(void);
int  pipe_read(int pipe_id, char *buf, int max_len);
int  pipe_write(int pipe_id, const char *buf, int len);
void pipe_close_write_end(int pipe_id);
void pipe_close_read_end(int pipe_id);
void pipe_close(int pipe_id);
void pipe_on_process_exit(int fd0, int fd1, int fd2);

#endif
