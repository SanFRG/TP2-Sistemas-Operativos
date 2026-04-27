#ifndef REG_PRINTER_H
#define REG_PRINTER_H

#include <stdint.h>
#include <exceptions.h>

void uint64_to_hex_string(uint64_t value, char* buffer);

void print_registers_graphic(RegisterFrame *frame, int x, int *y, uint32_t color, int scale);

#endif
