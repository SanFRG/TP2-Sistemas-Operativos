#include <shell.h>
#include <lib.h>
#include <test_exceptions.h>
#include <sound.h>

#define COLOR_BLACK   0x00000000
#define BUFFER_SIZE 256
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
