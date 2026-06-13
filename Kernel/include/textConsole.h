#ifndef TEXT_CONSOLE_H
#define TEXT_CONSOLE_H

#include <stdint.h>

#define TEXT_CONSOLE_WIDTH 80
#define TEXT_CONSOLE_HEIGHT 25

void tc_clear(void);
void tc_set_color(uint8_t attr);
void tc_put_char(char c);
void tc_write(const char *str, uint64_t length);

#endif
