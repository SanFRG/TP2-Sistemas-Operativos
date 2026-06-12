/* Motor de ejecucion de la shell: toma una linea ya parseada y la corre, sea
 * como proceso separado, builtin inline, o como un pipe "cmd1 | cmd2". El
 * catalogo de comandos y su lookup (find_cmd_fn) viven en shell_commands.c. */
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

/* Empaqueta la funcion del comando y sus argumentos en la estructura que
 * recibe pipe_process_entry cuando el proceso arranca. */
static void fill_pipe_args(pipe_proc_args_t *args, void (*fn)(int, char **),
                           int argc, char *argv[]) {
    args->fn = fn;
    args->argc = argc;
    for (int i = 0; i < argc; i++) {
        args->argv[i] = argv[i];
    }
    args->argv[argc] = 0; /* terminador NULL al estilo argv */
}

/* Ejecuta "cmd_izq | cmd_der": conecta el stdout del comando izquierdo con el
 * stdin del comando derecho a traves de un pipe, y espera a que ambos terminen. */
static void shell_execute_pipe(ShellCommandLine *line) {
    /* Resuelve cada nombre de comando a su funcion. */
    void (*left_fn)(int, char **)  = find_cmd_fn(line->argv[0]);
    void (*right_fn)(int, char **) = (line->argc2 > 0) ? find_cmd_fn(line->argv2[0]) : 0;

    if (left_fn == 0 || right_fn == 0) {
        println("Comando no encontrado para pipe.");
        return;
    }

    /* Crea el canal que comunica a ambos procesos. */
    int64_t pipe_id = pipe_open();
    if (pipe_id < 0) {
        println("Error: no se pudo crear el pipe.");
        return;
    }

    /* Prepara los argumentos con los que arrancara cada proceso. */
    pipe_proc_args_t left_args, right_args;
    fill_pipe_args(&left_args,  left_fn,  line->argc,  line->argv);
    fill_pipe_args(&right_args, right_fn, line->argc2, line->argv2);

    /* Constantes de configuracion de los procesos del pipe. */
    const int PRIORITY   = 1;
    const int BACKGROUND = 0;   /* no son foreground: la shell los espera */
    const int STDIN      = 0;
    const int STDOUT     = 1;

    /* Izquierdo: lee de stdin del usuario, escribe al pipe. */
    int64_t left_pid  = create_process_piped("pipe_left",  pipe_process_entry,
                                             &left_args,  PRIORITY, BACKGROUND,
                                             STDIN, (int)pipe_id);
    /* Derecho: lee del pipe, escribe a stdout. */
    int64_t right_pid = create_process_piped("pipe_right", pipe_process_entry,
                                             &right_args, PRIORITY, BACKGROUND,
                                             (int)pipe_id, STDOUT);

    if (left_pid < 0 || right_pid < 0) {
        println("Error al crear procesos del pipe.");
        /* Cerramos el pipe: libera el slot y desbloquea a quien estuviera
         * esperando. Si uno de los procesos si se creo, lo matamos y lo
         * esperamos para no dejar zombies ni procesos colgados. */
        pipe_close(pipe_id);
        if (left_pid >= 0)  { kill_process(left_pid);  waitpid(left_pid); }
        if (right_pid >= 0) { kill_process(right_pid); waitpid(right_pid); }
        return;
    }

    /* Espera a que ambos comandos terminen antes de devolver el prompt. */
    waitpid(left_pid);
    waitpid(right_pid);
}

/* Comandos que deben correr dentro del proceso shell (no como proceso
 * separado) porque modifican su estado interno. */
static int is_shell_builtin(const char *name) {
    return strcasecmp(name, "exit") == 0;
}

/* Lanza un comando como proceso separado. Empaqueta la funcion y una copia de
 * los argumentos en bg_cmd_args_t (cmd_process_entry los libera al terminar).
 * Devuelve el pid (>0) o -1 en caso de error, imprimiendo el motivo. */
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

    /* 'exit' corre inline: setea shell_exit en el propio proceso shell. */
    if (is_shell_builtin(line.argv[0])) {
        fn(line.argc, line.argv);
        return;
    }

    /* 'loop' es su propio lanzador: crea el proceso por dentro. shell_bg_flag
     * le indica si debe quedar en foreground o background. */
    if (strcasecmp(line.argv[0], "loop") == 0) {
        shell_bg_flag = line.background ? 1 : 0;
        fn(line.argc, line.argv);
        shell_bg_flag = 0;
        return;
    }

    /* Resto de los comandos: corren como proceso separado, igual que el
     * enunciado pide (no como built-ins dentro de la shell). */
    int64_t pid = launch_command_process(fn, &line, line.background ? 0 : 1);
    if (pid <= 0) {
        return;  /* launch_command_process ya informo el error */
    }

    if (line.background) {
        print("[");
        printInt((int)pid);
        print("] ");
        print(line.argv[0]);
        println(" iniciado en background.");
    } else {
        /* La shell espera a este proceso (ver handle_foreground_process). */
        shell_set_foreground_pid((int)pid);
    }
}
