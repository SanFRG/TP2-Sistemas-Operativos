#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_handler(void);

int hasNextKey(void);
int nextKey(void);
uint8_t getScancode(void);

char scancode_to_char(uint8_t scancode);

#endif