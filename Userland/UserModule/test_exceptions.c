#include "include/test_exceptions.h"
#include "include/lib.h"
#include <stdint.h>

void run_exception_test_zero_division(void) {
    // Usar volatile para evitar que el compilador optimice
    volatile int a = 42;
    volatile int b = 0;
    
    // Esto disparará la excepción 0x00
    a = a / b;
}

void run_exception_test_invalid_opcode(void) {
    // Llamar a la función en libasm.asm que ejecuta ud2
    trigger_invalid_opcode();
}