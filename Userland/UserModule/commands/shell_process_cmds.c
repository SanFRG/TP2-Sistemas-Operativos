#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <stdint.h>

static void loop_process_entry(void *arg) {
    int fg = (arg != 0) ? 1 : 0;
    int pid = (int)getpid();
    uint64_t ticks = get_ticks();
    uint64_t last_print = ticks;
    uint64_t interval = 100;

    while (1) {
        loop_inc();
        ticks = get_ticks();

        if (fg && ticks - last_print >= interval) {
            print("[loop] PID: ");
            printInt(pid);
            print(" count: ");
            println(" corriendo...");
            last_print = ticks;
        }

        yield_cpu();
    }
}

const char *shell_process_state_name(int state) {
    switch (state) {
        case PROCESS_READY:
            return "READY";
        case PROCESS_RUNNING:
            return "RUNNING";
        case PROCESS_BLOCKED:
            return "BLOCKED";
        case PROCESS_KILLED:
            return "KILLED";
        case PROCESS_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

void shell_print_int_padded(int n, int width) {
    char buf[16];
    int len = 0;
    int negative = 0;

    if (n < 0) {
        negative = 1;
        n = -n;
    }

    if (n == 0) {
        buf[len++] = '0';
    } else {
        while (n > 0) {
            buf[len++] = (char)('0' + (n % 10));
            n /= 10;
        }
    }

    if (negative) {
        buf[len++] = '-';
    }

    for (int i = len; i < width; i++) {
        print(" ");
    }

    for (int i = len - 1; i >= 0; i--) {
        char s[2] = {buf[i], 0};
        print(s);
    }
}

void cmd_ps(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    process_info entries[SHELL_MAX_PS_ENTRIES];
    int64_t count = ps(entries, SHELL_MAX_PS_ENTRIES);
    if (count < 0) {
        println("Error: no se pudo listar procesos.");
        return;
    }

    int max_count = 0;
    for (int i = 0; i < count; i++) {
        int c = (int)entries[i].loop_counter;
        if (c > max_count) {
            max_count = c;
        }
    }

    int count_width = 5;
    int tmp = max_count;
    while (tmp >= 10) {
        count_width++;
        tmp /= 10;
    }

    print("PID  PPID  PRIO FG  STATE");
    for (int i = 0; i < 6; i++) {
        print(" ");
    }
    for (int i = 5; i < count_width; i++) {
        print(" ");
    }
    println(" COUNT NAME");

    for (int i = 0; i < count; i++) {
        shell_print_int_padded(entries[i].pid, 3);
        print("  ");
        shell_print_int_padded(entries[i].parent_pid, 5);
        print(" ");
        shell_print_int_padded(entries[i].priority, 3);
        print("  ");
        shell_print_int_padded(entries[i].foreground, 2);
        print("  ");

        const char *state = shell_process_state_name(entries[i].state);
        print(state);
        int state_len = strlen(state);
        for (int j = 0; j < 11 - state_len; j++) {
            print(" ");
        }

        shell_print_int_padded((int)entries[i].loop_counter, count_width);
        print(" ");
        println(entries[i].name);
    }
}

void cmd_kill(int argc, char *argv[]) {
    if (argc < 2) {
        println("Uso: kill <pid>");
        return;
    }

    int pid = atoi(argv[1]);
    int my_pid = (int)getpid();
    int should_wait = 0;
    process_info entries[SHELL_MAX_PS_ENTRIES];
    int count = ps(entries, SHELL_MAX_PS_ENTRIES);

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            if (entries[i].pid == pid && entries[i].parent_pid == my_pid) {
                should_wait = 1;
                break;
            }
        }
    }

    if (kill_process(pid) == 0) {
        if (should_wait) {
            waitpid(pid);
        }
        print("Proceso ");
        printInt(pid);
        println(" terminado.");
    } else {
        print("Error: no se pudo matar el proceso ");
        printInt(pid);
        println(".");
    }
}

void cmd_nice(int argc, char *argv[]) {
    if (argc < 3) {
        println("Uso: nice <pid> <priority>");
        println("Prioridad: 0 (min) a 2 (max)");
        return;
    }

    int pid = atoi(argv[1]);
    int prio = atoi(argv[2]);
    if (prio < 0 || prio > 2) {
        println("Error: prioridad debe estar entre 0 y 2.");
        return;
    }

    if (nice_process(pid, prio) == 0) {
        print("Prioridad de ");
        printInt(pid);
        print(" cambiada a ");
        printInt(prio);
        println(".");
    } else {
        print("Error: no se pudo cambiar prioridad de ");
        printInt(pid);
        println(".");
    }
}

void cmd_block(int argc, char *argv[]) {
    if (argc < 2) {
        println("Uso: block <pid>");
        return;
    }

    int pid = atoi(argv[1]);
    process_info entries[SHELL_MAX_PS_ENTRIES];
    int count = ps(entries, SHELL_MAX_PS_ENTRIES);
    int found = 0;
    int current_state = -1;

    for (int i = 0; i < count; i++) {
        if (entries[i].pid == pid) {
            found = 1;
            current_state = entries[i].state;
            break;
        }
    }

    if (!found) {
        print("Error: proceso ");
        printInt(pid);
        println(" no encontrado.");
        return;
    }

    if (current_state == PROCESS_BLOCKED) {
        if (unblock_process(pid) == 0) {
            print("Proceso ");
            printInt(pid);
            println(" desbloqueado (READY).");
        } else {
            print("Error: no se pudo desbloquear ");
            printInt(pid);
            println(".");
        }
    } else if (current_state == PROCESS_READY || current_state == PROCESS_RUNNING) {
        if (block_process(pid) == 0) {
            print("Proceso ");
            printInt(pid);
            println(" bloqueado.");
        } else {
            print("Error: no se pudo bloquear ");
            printInt(pid);
            println(" (solo se pueden bloquear procesos READY/RUNNING).");
        }
    } else {
        print("Error: proceso ");
        printInt(pid);
        print(" esta en estado ");
        println(shell_process_state_name(current_state));
    }
}

void cmd_loop(int argc, char *argv[]) {
    int prio = 1;
    int fg = 1;
    int i = 1;

    while (i < argc) {
        if (argv[i][0] == '-' && argv[i][1] == 'p' && i + 1 < argc) {
            prio = atoi(argv[i + 1]);
            i += 2;
        } else if (argv[i][0] == '-' && argv[i][1] == 'b') {
            fg = 0;
            i++;
        } else {
            int val = atoi(argv[i]);
            if (i == 1) {
                prio = val;
            } else if (i == 2) {
                fg = val;
            }
            i++;
        }
    }

    if (prio < 0) {
        prio = 0;
    }
    if (prio > 2) {
        prio = 2;
    }

    int64_t pid = create_process("loop", loop_process_entry, fg ? (void *)1 : 0, prio, fg);
    if (pid < 0) {
        println("Error: no se pudo crear el proceso loop.");
        return;
    }

    print("Proceso loop creado con PID ");
    printInt((int)pid);
    println(".");
    if (fg) {
        shell_set_foreground_pid((int)pid);
    }
}
