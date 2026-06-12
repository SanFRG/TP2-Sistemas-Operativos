#include <shell.h>
#include <lib.h>
#include <stdint.h>

/* Portado de MemoryTest/test_prio.c de la catedra, adaptado a la API de
 * userland de este TP (create_process / nice_process / block_process /
 * unblock_process / waitpid). Demuestra que la prioridad afecta el reparto de
 * CPU: el scheduler es Round Robin y la prioridad modifica el quantum
 * (prio + 1), por lo que a mayor prioridad el proceso termina su cuenta antes
 * e imprime "DONE!" primero. */

#define TP_TOTAL_PROCESSES 3

#define TP_LOWEST  0
#define TP_MEDIUM  1
#define TP_HIGHEST 2

static const int tp_prio[TP_TOTAL_PROCESSES] = {TP_LOWEST, TP_MEDIUM, TP_HIGHEST};

/* Valor hasta el que cuenta cada proceso. Global compartido (mismo address
 * space para todos), igual que en la version de catedra. */
static uint64_t tp_max_value = 0;

/* Cuenta de 0 a tp_max_value y avisa al terminar. El orden en que aparecen los
 * "DONE!" refleja que prioridad recibio mas CPU. */
static void zero_to_max(void *arg) {
    (void)arg;
    volatile uint64_t value = 0;
    while (value++ != tp_max_value)
        ;

    print("PROCESS ");
    printInt((int)getpid());
    println(" DONE!");
}

void cmd_test_prio(int argc, char *argv[]) {
    int64_t pids[TP_TOTAL_PROCESSES];

    if (argc != 2) {
        println("Uso: test_prio <max_value>");
        return;
    }

    int parsed = atoi(argv[1]);
    if (parsed <= 0) {
        println("Uso: test_prio <max_value>");
        return;
    }
    tp_max_value = (uint64_t)parsed;

    /* --- Fase 1: misma prioridad --- */
    println("SAME PRIORITY...");
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        pids[i] = create_process("zero_to_max", zero_to_max, 0, TP_MEDIUM, 0);
    }
    /* Se espera que terminen aproximadamente al mismo tiempo. */
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }

    /* --- Fase 2: misma prioridad, luego se cambia --- */
    println("SAME PRIORITY, THEN CHANGE IT...");
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        pids[i] = create_process("zero_to_max", zero_to_max, 0, TP_MEDIUM, 0);
        nice_process(pids[i], tp_prio[i]);
        print("  PROCESS ");
        printInt((int)pids[i]);
        print(" NEW PRIORITY: ");
        printInt(tp_prio[i]);
        println("");
    }
    /* Se espera que la prioridad mas alta termine primero. */
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }

    /* --- Fase 3: se cambia la prioridad mientras estan bloqueados --- */
    println("SAME PRIORITY, THEN CHANGE IT WHILE BLOCKED...");
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        pids[i] = create_process("zero_to_max", zero_to_max, 0, TP_MEDIUM, 0);
        block_process(pids[i]);
        nice_process(pids[i], tp_prio[i]);
        print("  PROCESS ");
        printInt((int)pids[i]);
        print(" NEW PRIORITY: ");
        printInt(tp_prio[i]);
        println("");
    }
    /* Se desbloquean todos a la vez: arrancan con la prioridad ya seteada. */
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        unblock_process(pids[i]);
    }
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }

    println("test_prio: fin.");
}
