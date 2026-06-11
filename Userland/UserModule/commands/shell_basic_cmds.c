#include <shell.h>
#include <shell_internal.h>
#include <lib.h>
#include <sound.h>

void cmd_time(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    uint64_t packed = get_time();
    uint8_t ss = packed & 0xFF;
    uint8_t mm = (packed >> 8) & 0xFF;
    uint8_t hh = (packed >> 16) & 0xFF;
    uint8_t DD = (packed >> 24) & 0xFF;
    uint8_t MM = (packed >> 32) & 0xFF;
    uint8_t YY = (packed >> 40) & 0xFF;

    print("Fecha: ");
    print2Digits(DD);
    print("/");
    print2Digits(MM);
    print("/");
    printInt(2000 + YY);
    print("  ");

    print("Hora: ");
    print2Digits(hh);
    print(":");
    print2Digits(mm);
    print(":");
    print2Digits(ss);
    print("\n");
}

void cmd_pid(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    int64_t pid = getpid();
    if (pid < 0) {
        println("Error: no se pudo obtener el PID actual.");
        return;
    }

    print("PID actual: ");
    printInt((int)pid);
    print("\n");
}

void cmd_registers(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    RegisterSnapshot regs;
    if (get_regs(&regs) != 0) {
        println("\nError: No se pudieron obtener los registros\n");
        return;
    }

    println("\n=== Registros de CPU ===");

    print("RAX: ");
    printHex(regs.rax);
    println("");
    print("RBX: ");
    printHex(regs.rbx);
    println("");
    print("RCX: ");
    printHex(regs.rcx);
    println("");
    print("RDX: ");
    printHex(regs.rdx);
    println("");
    print("RSI: ");
    printHex(regs.rsi);
    println("");
    print("RDI: ");
    printHex(regs.rdi);
    println("");
    print("RBP: ");
    printHex(regs.rbp);
    println("");
    print("RSP: ");
    printHex(regs.rsp);
    println("");

    print("R8:  ");
    printHex(regs.r8);
    println("");
    print("R9:  ");
    printHex(regs.r9);
    println("");
    print("R10: ");
    printHex(regs.r10);
    println("");
    print("R11: ");
    printHex(regs.r11);
    println("");
    print("R12: ");
    printHex(regs.r12);
    println("");
    print("R13: ");
    printHex(regs.r13);
    println("");
    print("R14: ");
    printHex(regs.r14);
    println("");
    print("R15: ");
    printHex(regs.r15);
    println("");

    print("RIP: ");
    printHex(regs.rip);
    println("");
    print("RFLAGS: ");
    printHex(regs.rflags);
    println("");
    print("CS:  ");
    printHex(regs.cs);
    println("");
    print("SS:  ");
    printHex(regs.ss);
    println("");
    println("");
}

void cmd_clear(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    clear_screen(SHELL_COLOR_BLACK);
}

void cmd_cancion(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    mario_bros();
}
