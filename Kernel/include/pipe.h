#ifndef PIPE_H
#define PIPE_H

#include <stdint.h>

#define PIPE_BUFFER_SIZE 256
#define MAX_PIPES 8
#define PIPE_FD_MIN 3   /* pipe IDs start at 3; 0/1/2 reserved for stdin/stdout/stderr */

void pipe_system_init(void);
int  pipe_create(void);                                    /* returns pipe_id >= PIPE_FD_MIN, or -1 */
int  pipe_read(int pipe_id, char *buf, int max_len);       /* blocking; returns 0 on EOF */
int  pipe_write(int pipe_id, const char *buf, int len);    /* blocking; returns -1 on broken pipe */
void pipe_close_write_end(int pipe_id);
void pipe_close_read_end(int pipe_id);
void pipe_close(int pipe_id);                              /* close both ends and free the slot */
void pipe_on_process_exit(int fd0, int fd1, int fd2);      /* close pipe ends held by exiting process */

#endif
