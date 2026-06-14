#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <test_sync_userland.h>
#include <shell_io_cmds.h>

typedef struct {
    const char *name;
    void (*function)(int argc, char *argv[]);
} Command;

static Command commands[] = {
    {"help", cmd_help},
    {"time", cmd_time},
    {"mem", cmd_mem},
    {"pid", cmd_pid},
    {"ps", cmd_ps},
    {"memtest", cmd_memtest},
    {"testmm", cmd_testmm},
    {"regs", cmd_registers},
    {"clear", cmd_clear},
    {"cancion", cmd_cancion},
    {"loop", cmd_loop},
    {"kill", cmd_kill},
    {"nice", cmd_nice},
    {"block", cmd_block},
    {"testsync", cmd_testsync},
    {"testproc", cmd_testproc},
    {"testprio", cmd_testprio},
    {"mvar",      cmd_mvar},
    {"cat",       cmd_cat},
    {"wc",        cmd_wc},
    {"filter",    cmd_filter},
    {"exit", cmd_exit},
    {0, 0}
};

void cmd_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    println("  time              - Muestra la fecha y hora actual");
    println("  mem               - Muestra el estado del memory manager");
    println("  pid               - Muestra el PID del proceso actual");
    println("  ps                - Lista procesos activos (PID/PPID/PRIO/STATE)");
    println("  clear             - Limpia la pantalla");
    println("  cancion           - Reproduce Mario Bros por el PC speaker");
    println("  exit              - Sale de la shell");
    println("  loop [-p prio]    - Lanza proceso loop (espera activa)");
    println("  kill <pid>        - Mata un proceso");
    println("  nice <pid> <prio> - Cambia prioridad (0=min, 2=max)");
    println("  block <pid>       - Bloquea/desbloquea un proceso");
    println("  memtest           - alloc/free corto con resumen de 'mem'");
    println("  testmm <maxmem>   - Stress del MM. Pide/verifica/libera bloques aleatorios.");
    println("  testprio <n>      - 3 procesos cuentan hasta <n>. Mayor prio =>DONE! primero");
    println("  testproc <n>      - Crea/mata/bloquea <n> procesos al azar.");
    println("  testsync <p><n><s> - <p> pares inc/dec <n> veces. <s>=0 => race, <s>=1 => OK");
    println("  mvar <esc> <lec>  - Lectores/escritores sobre una MVar.");
    println("  cat               - Imprime stdin tal como lo recibe");
    println("  wc                - Cuenta la cantidad de lineas del input");
    println("  filter            - Filtra (elimina) las vocales del input");
    println("  &                 - Ejecuta el comando en background.");
    println("  |                 - Conecta stdout de cmd1 con stdin de cmd2.");
    println("  Ctrl+C            - Mata el proceso foreground");
    println("  Ctrl+D            - Envia EOF a la lectura actual");
}

shell_cmd_fn_t find_cmd_fn(char *name) {
    for (int i = 0; commands[i].name != 0; i++) {
        if (strcasecmp(name, commands[i].name) == 0) {
            return commands[i].function;
        }
    }
    return 0;
}

void cmd_exit(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    shell_exit = 1;
}
