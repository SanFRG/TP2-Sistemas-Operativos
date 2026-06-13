#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Scancodes de teclas extendidas (prefijo 0xE0)
#define EXTENDED_PREFIX     0xE0
#define ARROW_UP_SCANCODE   0x48
#define ARROW_DOWN_SCANCODE 0x50

// Códigos virtuales no imprimibles devueltos por scancode_to_char para las
// flechas (DC1/DC2: fuera del rango imprimible 32-126, no colisionan con
// '\n'/'\b'/'\t'). sys_read los interpreta para navegar el historial.
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