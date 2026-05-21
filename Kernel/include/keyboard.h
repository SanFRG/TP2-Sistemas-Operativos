#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_handler(void);

int hasNextKey(void);
int nextKey(void);
uint8_t getScancode(void);

char scancode_to_char(uint8_t scancode);

uint8_t get_and_clear_ctrl_c(void);
uint8_t get_and_clear_ctrl_d(void);
int is_ctrl_pressed(void);

#endif