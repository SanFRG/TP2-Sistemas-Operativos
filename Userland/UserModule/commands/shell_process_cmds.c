#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <stdint.h>

#define LOOP_PRINT_INTERVAL 100000

static void loop_process_entry(void *arg) {
    int fg = (arg != 0) ? 1 : 0;
    int pid = (int)getpid();
    uint64_t ticks = get_ticks();
    uint64_t last_print = ticks;

    while (1) {
        uint64_t count = loop_inc();
        ticks = get_ticks();

        if (fg && ticks - last_print >= LOOP_PRINT_INTERVAL) {
            print("[loop] PID: ");
            printInt(pid);
            print(" count: ");
            printInt((int)count);
            println("");
            last_print = ticks;
        }

        yield_cpu();
    }
}

static process_info *alloc_ps_entries(void) {
    return (process_info *)mem_alloc(sizeof(process_info) * SHELL_MAX_PS_ENTRIES);
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

static void ps_print_label(const char *s, int width) {
    print(s);
    for (int i = strlen(s); i < width; i++) {
        print(" ");
    }
}

static void ps_print_hex_field(uint64_t v, int width) {
    char buf[20];
    hexToString(v, buf);
    print("0x");
    print(buf);
    for (int i = 2 + strlen(buf); i < width; i++) {
        print(" ");
    }
}

void cmd_ps(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    process_info *entries = alloc_ps_entries();
    if (entries == 0) {
        println("Error: no hay memoria para listar procesos.");
        return;
    }

    int64_t count = ps(entries, SHELL_MAX_PS_ENTRIES);
    if (count < 0) {
        mem_free(entries);
        println("Error: no se pudo listar procesos.");
        return;
    }

    ps_print_label("PID", 4);   print("  ");
    ps_print_label("PPID", 4);  print("  ");
    ps_print_label("PRIO", 4);  print("  ");
    ps_print_label("FG", 2);    print("  ");
    ps_print_label("STATE", 11);print("  ");
    ps_print_label("SP", 13);   print("  ");
    ps_print_label("BP", 13);   print("  ");
    ps_print_label("COUNT", 5); print("  ");
    println("NAME");

    for (int i = 0; i < count; i++) {
        shell_print_int_padded(entries[i].pid, 4);
        print("  ");
        shell_print_int_padded(entries[i].parent_pid, 4);
        print("  ");
        shell_print_int_padded(entries[i].priority, 4);
        print("  ");
        shell_print_int_padded(entries[i].foreground, 2);
        print("  ");

        const char *state = shell_process_state_name(entries[i].state);
        ps_print_label(state, 11);
        print("  ");

        ps_print_hex_field(entries[i].stack_pointer, 13);
        print("  ");
        ps_print_hex_field(entries[i].base_pointer, 13);
        print("  ");

        shell_print_int_padded((int)entries[i].loop_counter, 5);
        print("  ");
        println(entries[i].name);
    }

    mem_free(entries);
}

void cmd_kill(int argc, char *argv[]) {
    if (argc < 2) {
        println("Uso: kill <pid>");
        return;
    }

    int pid = atoi(argv[1]);
    int my_pid = (int)getpid();
    int should_wait = 0;
    process_info *entries = alloc_ps_entries();
    int count = -1;

    if (entries != 0) {
        count = ps(entries, SHELL_MAX_PS_ENTRIES);
    }

    if (count > 0) {
        for (int i = 0; i < count; i++) {
            if (entries[i].pid == pid && entries[i].parent_pid == my_pid) {
                should_wait = 1;
                break;
            }
        }
    }
    if (entries != 0) {
        mem_free(entries);
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
    process_info *entries = alloc_ps_entries();
    if (entries == 0) {
        println("Error: no hay memoria para listar procesos.");
        return;
    }

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

    mem_free(entries);

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

    if (shell_bg_flag) {
        fg = 0;
    }

    while (i < argc) {
        if (argv[i][0] == '-' && argv[i][1] == 'p' && i + 1 < argc) {
            prio = atoi(argv[i + 1]);
            i += 2;
        } else {
            int val = atoi(argv[i]);
            if (i == 1) {
                prio = val;
            } else {
                break;
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
