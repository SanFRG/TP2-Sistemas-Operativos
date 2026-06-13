#include <shell.h>
#include <shell_internal.h>
#include <lib.h>

int shell_bg_flag = 0;

typedef struct {
    int argc;
    char *argv[SHELL_MAX_ARGS];
    void (*fn)(int, char **);
} pipe_proc_args_t;

static void pipe_process_entry(void *arg) {
    pipe_proc_args_t *a = (pipe_proc_args_t *)arg;
    a->fn(a->argc, a->argv);
}

static void cmd_process_entry(void *arg) {
    bg_cmd_args_t *a = (bg_cmd_args_t *)arg;
    char *argv[SHELL_MAX_ARGS];
    for (int i = 0; i < a->argc; i++) {
        argv[i] = a->argv[i];
    }
    argv[a->argc] = 0;
    a->fn(a->argc, argv);
    mem_free(a);
    exit_process(0);
}

static void fill_pipe_args(pipe_proc_args_t *args, void (*fn)(int, char **),
                           int argc, char *argv[]) {
    args->fn = fn;
    args->argc = argc;
    for (int i = 0; i < argc; i++) {
        args->argv[i] = argv[i];
    }
    args->argv[argc] = 0;
}

static void shell_execute_pipe(ShellCommandLine *line) {
    void (*left_fn)(int, char **)  = find_cmd_fn(line->argv[0]);
    void (*right_fn)(int, char **) = (line->argc2 > 0) ? find_cmd_fn(line->argv2[0]) : 0;

    if (left_fn == 0 || right_fn == 0) {
        println("Comando no encontrado para pipe.");
        return;
    }

    int64_t pipe_id = pipe_open();
    if (pipe_id < 0) {
        println("Error: no se pudo crear el pipe.");
        return;
    }

    pipe_proc_args_t left_args, right_args;
    fill_pipe_args(&left_args,  left_fn,  line->argc,  line->argv);
    fill_pipe_args(&right_args, right_fn, line->argc2, line->argv2);

    const int PRIORITY   = 1;
    const int BACKGROUND = 0;
    const int STDIN      = 0;
    const int STDOUT     = 1;

    int64_t left_pid  = create_process_piped("pipe_left",  pipe_process_entry,
                                             &left_args,  PRIORITY, BACKGROUND,
                                             STDIN, (int)pipe_id);
    int64_t right_pid = create_process_piped("pipe_right", pipe_process_entry,
                                             &right_args, PRIORITY, BACKGROUND,
                                             (int)pipe_id, STDOUT);

    if (left_pid < 0 || right_pid < 0) {
        println("Error al crear procesos del pipe.");
        pipe_close(pipe_id);
        if (left_pid >= 0)  { kill_process(left_pid);  waitpid(left_pid); }
        if (right_pid >= 0) { kill_process(right_pid); waitpid(right_pid); }
        return;
    }

    waitpid(left_pid);
    waitpid(right_pid);
}

static int is_shell_builtin(const char *name) {
    return strcasecmp(name, "exit") == 0;
}

static int64_t launch_command_process(void (*fn)(int, char **),
                                      ShellCommandLine *line, int foreground) {
    bg_cmd_args_t *args = mem_alloc(sizeof(bg_cmd_args_t));
    if (args == 0) {
        println("Error: no hay memoria para lanzar el proceso.");
        return -1;
    }

    args->fn = fn;
    args->argc = line->argc;
    for (int j = 0; j < line->argc; j++) {
        int k = 0;
        while (line->argv[j][k] != '\0' && k < SHELL_ARG_MAX - 1) {
            args->argv[j][k] = line->argv[j][k];
            k++;
        }
        args->argv[j][k] = '\0';
    }

    int64_t pid = create_process(line->argv[0], cmd_process_entry, args, 1,
                                 foreground);
    if (pid <= 0) {
        mem_free(args);
        println("Error: no se pudo crear el proceso.");
        return -1;
    }
    return pid;
}

void shell_execute_command(void) {
    ShellCommandLine line;

    if (shell_parse_line(buffer, &line) <= 0) {
        return;
    }

    if (line.has_pipe) {
        shell_execute_pipe(&line);
        return;
    }

    void (*fn)(int, char **) = find_cmd_fn(line.argv[0]);
    if (fn == 0) {
        println("Comando desconocido. Escribe 'help' para ver los comandos.");
        return;
    }

    if (is_shell_builtin(line.argv[0])) {
        fn(line.argc, line.argv);
        return;
    }

    if (strcasecmp(line.argv[0], "loop") == 0) {
        shell_bg_flag = line.background ? 1 : 0;
        fn(line.argc, line.argv);
        shell_bg_flag = 0;
        return;
    }

    int64_t pid = launch_command_process(fn, &line, line.background ? 0 : 1);
    if (pid <= 0) {
        return;
    }

    if (line.background) {
        print("[");
        printInt((int)pid);
        print("] ");
        print(line.argv[0]);
        println(" iniciado en background.");
    } else {
        shell_set_foreground_pid((int)pid);
    }
}
