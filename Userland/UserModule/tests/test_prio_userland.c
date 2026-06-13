#include <shell.h>
#include <lib.h>
#include <stdint.h>








#define TP_TOTAL_PROCESSES 3

#define TP_LOWEST  0
#define TP_MEDIUM  1
#define TP_HIGHEST 2

static const int tp_prio[TP_TOTAL_PROCESSES] = {TP_LOWEST, TP_MEDIUM, TP_HIGHEST};



static uint64_t tp_max_value = 0;



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


    println("SAME PRIORITY...");
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        pids[i] = create_process("zero_to_max", zero_to_max, 0, TP_MEDIUM, 0);
    }

    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }


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

    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }


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

    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        unblock_process(pids[i]);
    }
    for (int i = 0; i < TP_TOTAL_PROCESSES; i++) {
        waitpid(pids[i]);
    }

    println("test_prio: fin.");
}
