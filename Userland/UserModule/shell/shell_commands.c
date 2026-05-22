#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <test_sync_userland.h>

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
    println("  regs              - Muestra los registros guardados");
    println("  cerodiv           - Ejecuta division por cero");
    println("  invalido          - Dispara excepcion opcode invalido");
    println("  cancion           - Reproduce Mario Bros");
    println("  loop [prio] [fg]  - Lanza proceso loop (espera activa)");
    println("  kill <pid>        - Mata un proceso");
    println("  nice <pid> <prio> - Cambia prioridad (0-2)");
    println("  block <pid>       - Bloquea/desbloquea un proceso");
    println("  clear             - Limpia la pantalla");
    println("  exit              - Sale de la shell");
    println("");
    println("Modificadores:");
    println("  &                 - Ejecuta en background");
    println("  |                 - Pipes (pendiente)");
    println("  Ctrl+C            - Mata proceso foreground");
    println("  Ctrl+D            - EOF");
}

void shell_execute_command(void) {
    ShellCommandLine line;
    int found = 0;

    if (shell_parse_line(buffer, &line) <= 0) {
        return;
    }

    if (line.has_pipe) {
        println("Pipes no implementados.");
        return;
    }

    for (int i = 0; commands[i].name != 0 && !found; i++) {
        if (strcasecmp(line.argv[0], commands[i].name) == 0) {
            commands[i].function(line.argc, line.argv);
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
