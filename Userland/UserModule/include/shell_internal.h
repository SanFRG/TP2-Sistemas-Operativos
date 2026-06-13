#ifndef SHELL_INTERNAL_H
#define SHELL_INTERNAL_H

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16
#define SHELL_ARG_MAX 64
#define SHELL_MAX_PS_ENTRIES 64
#define SHELL_COLOR_BLACK 0x00000000

typedef enum {
    PROCESS_READY = 0,
    PROCESS_RUNNING = 1,
    PROCESS_BLOCKED = 2,
    PROCESS_KILLED = 3,
    PROCESS_TERMINATED = 4
} ShellProcessState;

typedef struct {
    int argc;
    char *argv[SHELL_MAX_ARGS];
    int background;
    int has_pipe;
    int argc2;
    char *argv2[SHELL_MAX_ARGS];
} ShellCommandLine;

typedef struct {
    void (*fn)(int, char **);
    int argc;
    char argv[SHELL_MAX_ARGS][SHELL_ARG_MAX];
} bg_cmd_args_t;



typedef void (*shell_cmd_fn_t)(int, char **);

extern int shell_exit;
extern int shell_bg_flag;
extern char buffer[SHELL_BUFFER_SIZE];

int shell_parse_line(char *line, ShellCommandLine *command);
void shell_execute_command(void);
shell_cmd_fn_t find_cmd_fn(char *name);
void shell_set_foreground_pid(int pid);
const char *shell_process_state_name(int state);
void shell_print_int_padded(int n, int width);

#endif
