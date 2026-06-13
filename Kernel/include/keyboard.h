#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>


#define EXTENDED_PREFIX     0xE0
#define ARROW_UP_SCANCODE   0x48
#define ARROW_DOWN_SCANCODE 0x50




#define KEY_UP   0x11
#define KEY_DOWN 0x12

void keyboard_handler(void);

int hasNextKey(void);
int nextKey(void);
uint8_t getScancode(void);

char scancode_to_char(uint8_t scancode);

uint8_t get_and_clear_ctrl_c(void);
uint8_t get_and_clear_ctrl_d(void);

#endif