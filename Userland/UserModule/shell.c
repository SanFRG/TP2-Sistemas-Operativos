#include <shell.h>
#include <lib.h>
#include <test_exceptions.h>
#include <sound.h>

#define COLOR_BLACK   0x00000000
#define BUFFER_SIZE 256
#define MAX_MM_BLOCKS 128
#define TEST_MM_ITERS 40
#define TEST_MM_MAX_MEMORY (128 * 1024)
#define NULL ((void*)0)
int shell_exit = 0;
char buffer[BUFFER_SIZE];

// Command structure
typedef struct {
    const char* name;
    void (*function)(void);
} Command;

// Command table (usando punteros a función)
static Command commands[] = {
    {"help", cmd_help},
    {"time", cmd_time},
    {"mem", cmd_mem},
    {"memtest", cmd_memtest},
    {"test_mm", cmd_test_mm},
    {"regs", cmd_registers},
    {"clear", cmd_clear},
    {"cerodiv", cmd_test_cero_division},
    {"invalido", cmd_test_invalid_opcode},
    {"cancion", cmd_cancion},
    {"loop", cmd_loop_test},
    {NULL, NULL}
};

void Bienvenido_a_la_Shell(void) {
    println("Bienvenido a la Shell");
    println("Escribe 'help' para ver los comandos disponibles\n");
}

void executeCommand() {
    
    // Search command in table and execute
    int found = 0;
    for (int i = 0; commands[i].name != NULL && !found; i++) {
        // Check for exit
        if (strcasecmp(buffer, "exit") == 0) {
            println("Saliendo de la shell...");
            shell_exit = 1;
            found = 1;
        } else if (strcasecmp(buffer, commands[i].name) == 0) {
            commands[i].function();  // Llamar mediante puntero a función
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
        // Print prompt
        print(">");
        
        // Read command from keyboard (blocking until Enter)
        int bytes_read = read(buffer, BUFFER_SIZE);

        if (bytes_read > 0) {
            executeCommand();
        }
    }
    println("Shell terminada.");
}

void cmd_help(void) {
    println("Comandos disponibles:");
    println("  help       - Muestra este mensaje de ayuda");
    println("  time       - Muestra la fecha y hora actual");
    println("  mem        - Muestra el estado del memory manager");
    println("  memtest    - Ejecuta alloc/free de prueba para el memory manager");
    println("  test_mm    - Test de stress de alloc/free (iteraciones acotadas)");
    println("  regs       - Muestra los registros guardados");
    println("  cerodiv    - Ejecuta la division por cero");
    println("  invalido   - Dispara excepcion por opcode invalido");
    println("  cancion    - Reproduce Mario Bros");
    println("  loop       - Loop infinito para testear captura de RIP");
    println("  clear      - Limpia la pantalla");
    println("  exit       - Sale de la shell");
}

void cmd_time(void) {
    uint64_t packed = get_time();   // [YY|MM|DD|hh|mm|ss]

    uint8_t ss =  packed        & 0xFF;
    uint8_t mm = (packed >> 8)  & 0xFF;
    uint8_t hh = (packed >> 16) & 0xFF;
    uint8_t DD = (packed >> 24) & 0xFF;
    uint8_t MM = (packed >> 32) & 0xFF;
    uint8_t YY = (packed >> 40) & 0xFF;

    // Mostrar fecha
    print("Fecha: ");
    print2Digits(DD); print("/");
    print2Digits(MM); print("/");
    printInt(2000 + YY);  // RTC da 0–99 → mostrar como 2000+YY
    print("  ");

    // Mostrar hora
    print("Hora: ");
    print2Digits(hh); print(":");
    print2Digits(mm); print(":");
    print2Digits(ss);
    print("\n");
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

void cmd_mem(void) {
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

void cmd_memtest(void) {
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

void cmd_test_mm(void) {
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

void cmd_registers(void) {
    RegisterSnapshot regs;
    
    // Get register snapshot from kernel
    if (get_regs(&regs) != 0) {
        println("\nError: No se pudieron obtener los registros\n");
        return;
    }
    
    println("\n=== Registros de CPU ===");
    
    // General purpose registers (64-bit)
    print("RAX: "); printHex(regs.rax); println("");
    print("RBX: "); printHex(regs.rbx); println("");
    print("RCX: "); printHex(regs.rcx); println("");
    print("RDX: "); printHex(regs.rdx); println("");
    print("RSI: "); printHex(regs.rsi); println("");
    print("RDI: "); printHex(regs.rdi); println("");
    print("RBP: "); printHex(regs.rbp); println("");
    print("RSP: "); printHex(regs.rsp); println("");
    
    // Extended registers
    print("R8:  "); printHex(regs.r8);  println("");
    print("R9:  "); printHex(regs.r9);  println("");
    print("R10: "); printHex(regs.r10); println("");
    print("R11: "); printHex(regs.r11); println("");
    print("R12: "); printHex(regs.r12); println("");
    print("R13: "); printHex(regs.r13); println("");
    print("R14: "); printHex(regs.r14); println("");
    print("R15: "); printHex(regs.r15); println("");
    
    // Special registers
    print("RIP: "); printHex(regs.rip); println("");
    print("RFLAGS: "); printHex(regs.rflags); println("");
    
    // Segment registers
    print("CS:  "); printHex(regs.cs); println("");
    print("SS:  "); printHex(regs.ss); println("");
    
    println("");
}

void cmd_clear(void) {
    clear_screen(COLOR_BLACK);  // Limpiar con fondo negro
}

void cmd_test_cero_division(void) {
    run_exception_test_zero_division();
}

void cmd_test_invalid_opcode(void) {
    run_exception_test_invalid_opcode();
}

void cmd_cancion(void) {
    mario_bros();
}

void cmd_loop_test(void) {
    println("Loop infinito iniciado. Presiona ESC en cualquier momento.");
    println("Luego presiona ESC de nuevo para salir y ver los registros con 'regs'.");
    
    volatile uint64_t counter = 0;
    while (1) {
        counter++;
        
        // Dibujar contador cada 100000 iteraciones para ver que está corriendo
        if (counter % 100000 == 0) {
            char msg[50];
            int idx = 0;
            msg[idx++] = 'C';
            msg[idx++] = 'o';
            msg[idx++] = 'u';
            msg[idx++] = 'n';
            msg[idx++] = 't';
            msg[idx++] = ':';
            msg[idx++] = ' ';
            
            // Convertir número a string
            char num[20];
            itoa(counter / 100000, num);
            int i = 0;
            while (num[i]) {
                msg[idx++] = num[i++];
            }
            msg[idx++] = '\n';
            msg[idx] = '\0';
            
            print(msg);
        }
        
        // Chequear si presionaron ESC para salir
        int key = get_key();
        if (key == 0x01) {  // ESC scancode
            println("\nLoop terminado. Usa 'regs' para ver los registros capturados.");
            break;
        }
    }
}
