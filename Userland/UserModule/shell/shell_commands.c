#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <test_sync_userland.h>
#include <shell_pipe.h>

typedef struct {
    const char *name;
    void (*function)(int argc, char *argv[]);
} Command;

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

static Command commands[] = {
    {"help", cmd_help},
    {"time", cmd_time},
    {"mem", cmd_mem},
    {"pid", cmd_pid},
    {"ps", cmd_ps},
    {"memtest", cmd_memtest},
    {"test_mm", cmd_test_mm},
    {"regs", cmd_registers},
    {"clear", cmd_clear},
    {"cerodiv", cmd_test_cero_division},
    {"invalido", cmd_test_invalid_opcode},
    {"cancion", cmd_cancion},
    {"loop", cmd_loop},
    {"kill", cmd_kill},
    {"nice", cmd_nice},
    {"block", cmd_block},
    {"test_sync", cmd_test_sync},
    {"cat",       cmd_cat},
    {"wc",        cmd_wc},
    {"filter",    cmd_filter},
    {"exit", cmd_exit},
    {0, 0}
};

void cmd_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    println("Comandos disponibles:");
    println("  help              - Muestra este mensaje de ayuda");
    println("  time              - Muestra la fecha y hora actual");
    println("  mem               - Muestra el estado del memory manager");
    println("  pid               - Muestra el PID del proceso actual");
    println("  ps                - Lista procesos activos");
    println("  memtest           - Ejecuta alloc/free de prueba");
    println("  test_mm           - Test de stress de alloc/free");
    println("  test_sync <n> <s> - Test de sincronizacion (s=0/1)");
    println("  cancion           - Reproduce Mario Bros");
    println("  loop [-p prio]    - Lanza proceso loop (espera activa)");
    println("  kill <pid>        - Mata un proceso");
    println("  nice <pid> <prio> - Cambia prioridad (0-2)");
    println("  block <pid>       - Bloquea/desbloquea un proceso");
    println("  clear             - Limpia la pantalla");
    println("  cat               - Imprime stdin tal como lo recibe");
    println("  wc                - Cuenta la cantidad de lineas del input");
    println("  filter            - Filtra las vocales del input");
    println("  exit              - Sale de la shell");
    println("Modificadores:");
    println("  &                 - Ejecuta en background");
    println("  |                 - Conecta stdout de cmd1 con stdin de cmd2");
    println("  Ctrl+C            - Mata proceso foreground");
    println("  Ctrl+D            - EOF");
}

static void (*find_cmd_fn(char *name))(int, char **) {
    for (int i = 0; commands[i].name != 0; i++) {
        if (strcasecmp(name, commands[i].name) == 0) {
            return commands[i].function;
        }
    }
    return 0;
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

void shell_execute_command(void) {
    ShellCommandLine line;
    int found = 0;

    if (shell_parse_line(buffer, &line) <= 0) {
        return;
    }

    if (line.has_pipe) {
        shell_execute_pipe(&line);
        return;
    }

    for (int i = 0; commands[i].name != 0 && !found; i++) {
        if (strcasecmp(line.argv[0], commands[i].name) == 0) {
            if (line.background) {
                if (strcasecmp(line.argv[0], "exit") == 0) {
                    commands[i].function(line.argc, line.argv);
                    found = 1;
                    break;
                }

                if (strcasecmp(line.argv[0], "loop") == 0) {
                    shell_bg_flag = 1;
                    commands[i].function(line.argc, line.argv);
                    shell_bg_flag = 0;
                    found = 1;
                    break;
                }

                bg_cmd_args_t *bg = mem_alloc(sizeof(bg_cmd_args_t));
                if (bg == 0) {
                    println("Error: no hay memoria para lanzar proceso en background.");
                    found = 1;
                    break;
                }
                bg->fn = commands[i].function;
                bg->argc = line.argc;
                for (int j = 0; j < line.argc; j++) {
                    int k = 0;
                    while (line.argv[j][k] != '\0' && k < SHELL_ARG_MAX - 1) {
                        bg->argv[j][k] = line.argv[j][k];
                        k++;
                    }
                    bg->argv[j][k] = '\0';
                }

                int64_t pid = create_process(line.argv[0], cmd_process_entry, bg, 1, 0);
                if (pid > 0) {
                    print("["); printInt((int)pid); print("] ");
                    print(line.argv[0]); println(" iniciado en background.");
                } else {
                    mem_free(bg);
                    println("Error: no se pudo crear proceso en background.");
                }
            } else {
                commands[i].function(line.argc, line.argv);
            }
            found = 1;
        }
    }

    if (!found) {
        println("Comando desconocido. Escribe 'help' para ver los comandos.");
    }
}

void cmd_exit(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    shell_exit = 1;
}
