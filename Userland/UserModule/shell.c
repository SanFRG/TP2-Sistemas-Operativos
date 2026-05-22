#include <shell.h>
#include <lib.h>
#include <test_exceptions.h>
#include <sound.h>

#define COLOR_BLACK   0x00000000
#define BUFFER_SIZE 256
#define MAX_MM_BLOCKS 128
#define TEST_MM_ITERS 40
#define TEST_MM_MAX_MEMORY (128 * 1024)
#define MAX_PS_ENTRIES 64
#define MAX_ARGS 16
#define NULL ((void*)0)

int shell_exit = 0;
char buffer[BUFFER_SIZE];

// PID del proceso en foreground (0 = ninguno)
static int fg_pid = 0;

// Command structure con soporte de argumentos
typedef struct {
    const char* name;
    void (*function)(int argc, char *argv[]);
} Command;

// ============================================================
// Loop process entry point (runs as separate process)
// ============================================================

static void loop_process_entry(void *arg) {
    int fg = (arg != NULL) ? 1 : 0;
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
            // no podemos leer el contador desde aca, solo imprimimos el PID
            println(" corriendo...");
            last_print = ticks;
        }

        yield_cpu();
    }
}

// ============================================================
// Command table
// ============================================================

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
    {NULL, NULL}
};

// ============================================================
// Shell main loop
// ============================================================

void Bienvenido_a_la_Shell(void) {
    println("Bienvenido a la Shell");
    println("Escribe 'help' para ver los comandos disponibles\n");
}

void executeCommand() {
    // Parsear argumentos: tokenizar buffer por espacios
    char *argv[MAX_ARGS];
    int argc = 0;
    char *p = buffer;

    while (*p && argc < MAX_ARGS - 1) {
        // Skip leading spaces
        while (*p == ' ') p++;
        if (*p == '\0') break;

        // Check for & at end (background) - consumed but not yet fully implemented
        if (*p == '&' && (p[1] == '\0' || p[1] == ' ')) {
            break;
        }

        argv[argc++] = p;

        // Find end of token
        while (*p && *p != ' ') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    // Buscar comando y ejecutar
    int found = 0;
    for (int i = 0; commands[i].name != NULL && !found; i++) {
        if (strcasecmp(argv[0], commands[i].name) == 0) {
            commands[i].function(argc, argv);
            found = 1;
        }
    }

    if (!found) {
        println("Comando desconocido. Escribe 'help' para ver los comandos.");
    }
}

void shell() {
    Bienvenido_a_la_Shell();
    while (!shell_exit) {
        // Si hay un proceso foreground, esperar a que termine
        if (fg_pid != 0) {
            // Verificar si el proceso sigue vivo
            process_info entries[MAX_PS_ENTRIES];
            int count = ps(entries, MAX_PS_ENTRIES);
            int alive = 0;
            for (int i = 0; i < count; i++) {
                if (entries[i].pid == fg_pid &&
                    entries[i].state != 3 &&  // KILLED
                    entries[i].state != 4) {  // TERMINATED
                    alive = 1;
                    break;
                }
            }

            if (alive) {
                // Check Ctrl+C para matar proceso foreground
                if (check_ctrl_c()) {
                    print("\nCtrl+C detectado. Matando proceso ");
                    printInt(fg_pid);
                    println("...");
                    kill_process(fg_pid);
                } else {
                    yield_cpu();
                    continue;
                }
            }

            // Recolectar el proceso terminado
            waitpid(fg_pid);
            fg_pid = 0;
        }

        // Print prompt
        print("> ");

        // Read command from keyboard (blocking until Enter)
        int64_t bytes_read = read(buffer, BUFFER_SIZE);

        if (bytes_read < 0) {
            // Ctrl+C durante la lectura
            if (fg_pid != 0) {
                print("\nCtrl+C detectado. Matando proceso ");
                printInt(fg_pid);
                println("...");
                kill_process(fg_pid);
                waitpid(fg_pid);
                fg_pid = 0;
            }
            println("");
            continue;
        }

        if (bytes_read == 0) {
            // Ctrl+D (EOF) o linea vacia
            continue;
        }

        executeCommand();
    }
    println("Shell terminada.");
}

// ============================================================
// Command implementations
// ============================================================

void cmd_help(int argc, char *argv[]) {
    (void)argc; (void)argv;
    println("Comandos disponibles:");
    println("  help              - Muestra este mensaje de ayuda");
    println("  time              - Muestra la fecha y hora actual");
    println("  mem               - Muestra el estado del memory manager");
    println("  pid               - Muestra el PID del proceso actual");
    println("  ps                - Lista procesos activos");
    println("  memtest           - Ejecuta alloc/free de prueba");
    println("  test_mm           - Test de stress de alloc/free");
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
    println("  Ctrl+C            - Mata proceso foreground");
    println("  Ctrl+D            - EOF");
}

void cmd_time(int argc, char *argv[]) {
    (void)argc; (void)argv;
    uint64_t packed = get_time();

    uint8_t ss =  packed        & 0xFF;
    uint8_t mm = (packed >> 8)  & 0xFF;
    uint8_t hh = (packed >> 16) & 0xFF;
    uint8_t DD = (packed >> 24) & 0xFF;
    uint8_t MM = (packed >> 32) & 0xFF;
    uint8_t YY = (packed >> 40) & 0xFF;

    print("Fecha: ");
    print2Digits(DD); print("/");
    print2Digits(MM); print("/");
    printInt(2000 + YY);
    print("  ");

    print("Hora: ");
    print2Digits(hh); print(":");
    print2Digits(mm); print(":");
    print2Digits(ss);
    print("\n");
}

void cmd_pid(int argc, char *argv[]) {
    (void)argc; (void)argv;
    int64_t pid = getpid();
    if (pid < 0) {
        println("Error: no se pudo obtener el PID actual.");
        return;
    }

    print("PID actual: ");
    printInt((int)pid);
    print("\n");
}

static const char *state_to_str(int state) {
    switch (state) {
        case 0: return "READY";
        case 1: return "RUNNING";
        case 2: return "BLOCKED";
        case 3: return "KILLED";
        case 4: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

static void printIntPadded(int n, int width) {
    char buf[16];
    int len = 0;
    int negative = 0;
    if (n < 0) { negative = 1; n = -n; }
    if (n == 0) buf[len++] = '0';
    else while (n > 0) { buf[len++] = '0' + (n % 10); n /= 10; }
    if (negative) buf[len++] = '-';
    for (int i = len; i < width; i++) print(" ");
    for (int i = len - 1; i >= 0; i--) { char s[2] = {buf[i], 0}; print(s); }
}

void cmd_ps(int argc, char *argv[]) {
    (void)argc; (void)argv;
    process_info entries[MAX_PS_ENTRIES];
    int64_t count = ps(entries, MAX_PS_ENTRIES);

    if (count < 0) {
        println("Error: no se pudo listar procesos.");
        return;
    }

    int max_count = 0;
    for (int i = 0; i < count; i++) {
        int c = (int)entries[i].loop_counter;
        if (c > max_count) max_count = c;
    }
    int count_width = 5;
    int tmp = max_count;
    while (tmp >= 10) { count_width++; tmp /= 10; }

    print("PID  PPID  PRIO FG  STATE");
    for (int i = 0; i < 11 - 5; i++) print(" ");   // pad "STATE" to 11
    for (int i = 5; i < count_width; i++) print(" "); // pad "COUNT" to count_width
    println(" COUNT NAME");
    for (int i = 0; i < count; i++) {
        printIntPadded(entries[i].pid, 3);        print("  ");
        printIntPadded(entries[i].parent_pid, 5); print(" ");
        printIntPadded(entries[i].priority, 3);   print("  ");
        printIntPadded(entries[i].foreground, 2); print("  ");
        const char *st = state_to_str(entries[i].state);
        print(st);
        int state_len = strlen(st);
        for (int j = 0; j < 11 - state_len; j++) print(" ");
        printIntPadded((int)entries[i].loop_counter, count_width); print(" ");
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
    process_info entries[MAX_PS_ENTRIES];
    int count = ps(entries, MAX_PS_ENTRIES);

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
            // Recolectar hijo matado para liberar stack y slot del PCB.
            // El exit_code puede ser -1 (kill), por eso no usamos el retorno.
            waitpid(pid);
        }
        print("Proceso "); printInt(pid); println(" terminado.");
    } else {
        print("Error: no se pudo matar el proceso "); printInt(pid); println(".");
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
        print("Prioridad de "); printInt(pid);
        print(" cambiada a "); printInt(prio); println(".");
    } else {
        print("Error: no se pudo cambiar prioridad de "); printInt(pid); println(".");
    }
}

void cmd_block(int argc, char *argv[]) {
    if (argc < 2) {
        println("Uso: block <pid>");
        return;
    }
    int pid = atoi(argv[1]);

    // Consultar estado actual via ps
    process_info entries[MAX_PS_ENTRIES];
    int count = ps(entries, MAX_PS_ENTRIES);

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
        print("Error: proceso "); printInt(pid); println(" no encontrado.");
        return;
    }

    if (current_state == 2) {  // BLOCKED
        if (unblock_process(pid) == 0) {
            print("Proceso "); printInt(pid); println(" desbloqueado (READY).");
        } else {
            print("Error: no se pudo desbloquear "); printInt(pid); println(".");
        }
    } else if (current_state == 0 || current_state == 1) {  // READY o RUNNING
        if (block_process(pid) == 0) {
            print("Proceso "); printInt(pid); println(" bloqueado.");
        } else {
            print("Error: no se pudo bloquear "); printInt(pid);
            println(" (solo se pueden bloquear procesos READY/RUNNING).");
        }
    } else {
        print("Error: proceso "); printInt(pid);
        print(" esta en estado "); println(state_to_str(current_state));
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
            if (i == 1) prio = val;
            else if (i == 2) fg = val;
            i++;
        }
    }

    if (prio < 0) prio = 0;
    if (prio > 2) prio = 2;

    int64_t pid = create_process("loop", loop_process_entry, fg ? (void*)1 : NULL, prio, fg);
    if (pid < 0) {
        println("Error: no se pudo crear el proceso loop.");
    } else {
        print("Proceso loop creado con PID "); printInt((int)pid); println(".");
        if (fg) {
            fg_pid = (int)pid;
        }
    }
}

typedef struct {
    void *address;
    uint32_t size;
} mm_rq_t;

static uint32_t mm_rand_z = 362436069U;
static uint32_t mm_rand_w = 521288629U;

static uint32_t mm_rand_u32(void) {
    mm_rand_z = 36969U * (mm_rand_z & 65535U) + (mm_rand_z >> 16);
    mm_rand_w = 18000U * (mm_rand_w & 65535U) + (mm_rand_w >> 16);
    return (mm_rand_z << 16) + mm_rand_w;
}

static uint32_t mm_uniform(uint32_t max) {
    if (max == 0) {
        return 0;
    }
    return (uint32_t)((mm_rand_u32() + 1.0) * 2.328306435454494e-10 * max);
}

static void mm_fill(void *start, uint8_t value, uint32_t size) {
    uint8_t *p = (uint8_t *)start;
    for (uint32_t i = 0; i < size; i++) {
        p[i] = value;
    }
}

static uint8_t mm_check(const void *start, uint8_t value, uint32_t size) {
    const uint8_t *p = (const uint8_t *)start;
    for (uint32_t i = 0; i < size; i++) {
        if (p[i] != value) {
            return 0;
        }
    }
    return 1;
}

void cmd_mem(int argc, char *argv[]) {
    (void)argc; (void)argv;
    MemoryStatus status;

    if (mem_status(&status) != 0) {
        println("Error: no se pudo obtener estado de memoria.");
        return;
    }

    println("=== Estado de memoria ===");
    print("Total bytes: "); printHex(status.total_bytes); print("\n");
    print("Used bytes:  "); printHex(status.used_bytes); print("\n");
    print("Free bytes:  "); printHex(status.free_bytes); print("\n");
    print("Allocs ok:   "); printHex(status.successful_allocations); print("\n");
    print("Frees ok:    "); printHex(status.successful_frees); print("\n");
    print("Allocs fail: "); printHex(status.failed_allocations); print("\n");
}

void cmd_memtest(int argc, char *argv[]) {
    (void)argc; (void)argv;
    MemoryStatus before;
    MemoryStatus after_alloc;
    MemoryStatus after_free;
    void *p1;
    void *p2;

    if (mem_status(&before) != 0) {
        println("memtest: no se pudo leer estado inicial.");
        return;
    }

    p1 = mem_alloc(64);
    p2 = mem_alloc(256);

    if (mem_status(&after_alloc) != 0) {
        println("memtest: no se pudo leer estado despues de alloc.");
        return;
    }

    if (p1 != 0) {
        mem_free(p1);
    }
    if (p2 != 0) {
        mem_free(p2);
    }

    if (mem_status(&after_free) != 0) {
        println("memtest: no se pudo leer estado final.");
        return;
    }

    println("=== memtest ===");
    print("p1(64)  = "); printHex((uint64_t)p1); print("\n");
    print("p2(256) = "); printHex((uint64_t)p2); print("\n");

    println("--- before ---");
    print("used: "); printHex(before.used_bytes);
    print(" free: "); printHex(before.free_bytes);
    print(" allocs: "); printHex(before.successful_allocations);
    print(" frees: "); printHex(before.successful_frees);
    print(" fails: "); printHex(before.failed_allocations);
    print("\n");

    println("--- after alloc ---");
    print("used: "); printHex(after_alloc.used_bytes);
    print(" free: "); printHex(after_alloc.free_bytes);
    print(" allocs: "); printHex(after_alloc.successful_allocations);
    print(" frees: "); printHex(after_alloc.successful_frees);
    print(" fails: "); printHex(after_alloc.failed_allocations);
    print("\n");

    println("--- after free ---");
    print("used: "); printHex(after_free.used_bytes);
    print(" free: "); printHex(after_free.free_bytes);
    print(" allocs: "); printHex(after_free.successful_allocations);
    print(" frees: "); printHex(after_free.successful_frees);
    print(" fails: "); printHex(after_free.failed_allocations);
    print("\n");
}

void cmd_test_mm(int argc, char *argv[]) {
    (void)argc; (void)argv;
    mm_rq_t reqs[MAX_MM_BLOCKS];
    uint32_t total;
    uint8_t rq;
    uint32_t i;

    println("test_mm: iniciando (iteraciones acotadas)...");

    for (int iter = 0; iter < TEST_MM_ITERS; iter++) {
        rq = 0;
        total = 0;

        while (rq < MAX_MM_BLOCKS && total < TEST_MM_MAX_MEMORY) {
            uint32_t remaining = TEST_MM_MAX_MEMORY - total;
            uint32_t size = mm_uniform(remaining);
            if (size == 0) {
                size = 1;
            }

            reqs[rq].size = size;
            reqs[rq].address = mem_alloc(size);
            if (reqs[rq].address != 0) {
                total += size;
                rq++;
            } else {
                break;
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0) {
                mm_fill(reqs[i].address, (uint8_t)i, reqs[i].size);
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0 && !mm_check(reqs[i].address, (uint8_t)i, reqs[i].size)) {
                println("test_mm ERROR: corrupcion detectada");
                return;
            }
        }

        for (i = 0; i < rq; i++) {
            if (reqs[i].address != 0) {
                mem_free(reqs[i].address);
            }
        }
    }

    println("test_mm OK");
}

void cmd_registers(int argc, char *argv[]) {
    (void)argc; (void)argv;
    RegisterSnapshot regs;

    if (get_regs(&regs) != 0) {
        println("\nError: No se pudieron obtener los registros\n");
        return;
    }

    println("\n=== Registros de CPU ===");

    print("RAX: "); printHex(regs.rax); println("");
    print("RBX: "); printHex(regs.rbx); println("");
    print("RCX: "); printHex(regs.rcx); println("");
    print("RDX: "); printHex(regs.rdx); println("");
    print("RSI: "); printHex(regs.rsi); println("");
    print("RDI: "); printHex(regs.rdi); println("");
    print("RBP: "); printHex(regs.rbp); println("");
    print("RSP: "); printHex(regs.rsp); println("");

    print("R8:  "); printHex(regs.r8);  println("");
    print("R9:  "); printHex(regs.r9);  println("");
    print("R10: "); printHex(regs.r10); println("");
    print("R11: "); printHex(regs.r11); println("");
    print("R12: "); printHex(regs.r12); println("");
    print("R13: "); printHex(regs.r13); println("");
    print("R14: "); printHex(regs.r14); println("");
    print("R15: "); printHex(regs.r15); println("");

    print("RIP: "); printHex(regs.rip); println("");
    print("RFLAGS: "); printHex(regs.rflags); println("");

    print("CS:  "); printHex(regs.cs); println("");
    print("SS:  "); printHex(regs.ss); println("");

    println("");
}

void cmd_clear(int argc, char *argv[]) {
    (void)argc; (void)argv;
    clear_screen(COLOR_BLACK);
}

void cmd_test_cero_division(int argc, char *argv[]) {
    (void)argc; (void)argv;
    run_exception_test_zero_division();
}

void cmd_test_invalid_opcode(int argc, char *argv[]) {
    (void)argc; (void)argv;
    run_exception_test_invalid_opcode();
}

void cmd_cancion(int argc, char *argv[]) {
    (void)argc; (void)argv;
    mario_bros();
}
