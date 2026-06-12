#include <shell.h>
#include <shell_internal.h>
#include <lib.h>

int shell_exit = 0;
char buffer[SHELL_BUFFER_SIZE];

static int fg_pid = 0;

void shell_set_foreground_pid(int pid) {
    fg_pid = pid;
}

static void print_welcome(void) {
    set_color(COLOR_CYAN);
    println("");
    println("  +==================================================+");
    println("  |                                                  |");
    set_color(COLOR_WHITE);
    println("  |            B I E N V E N I D O   A               |");
    set_color(COLOR_GREEN);
    println("  |             L A   S H E L L  -  T P 2            |");
    set_color(COLOR_CYAN);
    println("  |                                                  |");
    println("  +==================================================+");
    set_color(COLOR_YELLOW);
    println("");
    println("  Escribe 'help' para ver los comandos disponibles");
    set_color(COLOR_DEFAULT);
    println("");
}

static int foreground_is_alive(void) {
    process_info *entries = (process_info *)mem_alloc(sizeof(process_info) * SHELL_MAX_PS_ENTRIES);
    if (entries == 0) {
        return 0;
    }

    int count = ps(entries, SHELL_MAX_PS_ENTRIES);

    if (count <= 0) {
        mem_free(entries);
        return 0;
    }

    for (int i = 0; i < count; i++) {
        if (entries[i].pid == fg_pid &&
            entries[i].state != PROCESS_KILLED &&
            entries[i].state != PROCESS_TERMINATED) {
            mem_free(entries);
            return 1;
        }
    }

    mem_free(entries);
    return 0;
}

static void kill_foreground_process(void) {
    print("\nCtrl+C detectado. Matando proceso ");
    printInt(fg_pid);
    println("...");
    kill_process(fg_pid);
}

static int handle_foreground_process(void) {
    if (fg_pid == 0) {
        return 0;
    }

    if (foreground_is_alive()) {
        if (check_ctrl_c()) {
            kill_foreground_process();
        } else {
            yield_cpu();
            return 1;
        }
    }

    waitpid(fg_pid);
    fg_pid = 0;
    return 0;
}

void shell(void) {
    print_welcome();

    while (!shell_exit) {
        if (handle_foreground_process()) {
            continue;
        }

        print("> ");

        int64_t bytes_read = read(buffer, SHELL_BUFFER_SIZE);
        if (bytes_read < 0) {
            if (fg_pid != 0) {
                kill_foreground_process();
                waitpid(fg_pid);
                fg_pid = 0;
            }
            println("");
            continue;
        }

        if (bytes_read == 0) {
            continue;
        }

        shell_execute_command();
    }

    println("Shell terminada.");
}
