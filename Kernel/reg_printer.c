#include <reg_printer.h>
#include <interrupts.h>
#include <textConsole.h>

static uint64_t str_len(const char *str) {
    uint64_t len = 0;
    while (str[len] != 0) {
        len++;
    }
    return len;
}

static void print_line(const char *line) {
    tc_write(line, str_len(line));
    tc_write("\n", 1);
}

void uint64_to_hex_string(uint64_t value, char* buffer) {
    for (int i = 15; i >= 0; i--) {
        int digit = (value >> (i * 4)) & 0xF;
        buffer[15 - i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
    }
    buffer[16] = '\0';
}

// Copia un string a otro buffer (retorna puntero al final)
static char* append_string(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    return dest;
}

static void print_register_pair(const char* name1, uint64_t val1, 
                                const char* name2, uint64_t val2,
                                int x, int *y, uint32_t color, int scale) {
    (void)x;
    (void)color;
    (void)scale;

    char line[80];  // Buffer para construir la línea completa
    char hex1[17], hex2[17];
    
    uint64_to_hex_string(val1, hex1);
    
    char* p = line;
    p = append_string(p, name1);
    p = append_string(p, hex1);
    
    if (name2 && *name2) {
        uint64_to_hex_string(val2, hex2);
        
        while (p - line < 35) *p++ = ' ';
        
        p = append_string(p, name2);
        p = append_string(p, hex2);
    }
    
    *p = '\0';
    
    print_line(line);
    (*y)++;
}

void print_registers_graphic(RegisterFrame *frame, int x, int *y, uint32_t color, int scale) {
    (void)x;
    (void)color;
    (void)scale;

    char line[80];
    char hex[17];
    
    uint64_to_hex_string(frame->rip, hex);
    char* p = line;
    p = append_string(p, "Instruction Pointer (RIP): 0x");
    p = append_string(p, hex);
    *p = '\0';
    print_line(line);
    (*y)++;
    
    print_line("");
    print_line("Registros del procesador:");
    (*y) += 2;
    
    print_register_pair("RAX: 0x", frame->rax, "RBX: 0x", frame->rbx, x, y, color, scale);
    print_register_pair("RCX: 0x", frame->rcx, "RDX: 0x", frame->rdx, x, y, color, scale);
    print_register_pair("RSI: 0x", frame->rsi, "RDI: 0x", frame->rdi, x, y, color, scale);
    print_register_pair("RBP: 0x", frame->rbp, "RSP: 0x", frame->rsp, x, y, color, scale);
    print_register_pair("R8 : 0x", frame->r8,  "R9 : 0x", frame->r9,  x, y, color, scale);
    print_register_pair("R10: 0x", frame->r10, "R11: 0x", frame->r11, x, y, color, scale);
    print_register_pair("R12: 0x", frame->r12, "R13: 0x", frame->r13, x, y, color, scale);
    print_register_pair("R14: 0x", frame->r14, "R15: 0x", frame->r15, x, y, color, scale);
    
    // Línea en blanco
    print_line("");
    (*y)++;
    
    print_register_pair("CS: 0x", frame->cs, "SS: 0x", frame->ss, x, y, color, scale);
    print_register_pair("RFLAGS: 0x", frame->rflags, "", 0, x, y, color, scale);
}
