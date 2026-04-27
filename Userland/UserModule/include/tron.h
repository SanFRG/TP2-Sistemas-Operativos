#ifndef TRON_H
#define TRON_H

// Scancodes para controles del juego
#define KEY_W 0x11
#define KEY_A 0x1E
#define KEY_S 0x1F
#define KEY_D 0x20

#define KEY_I 0x17
#define KEY_J 0x24
#define KEY_K 0x25
#define KEY_L 0x26

#define KEY_ESC 0x01
#define KEY_ENTER 0x1C
#define KEY_SPACE 0x39
#define KEY_1 0x02
#define KEY_2 0x03
#define KEY_Q 0x10

// Colores (formato RGB 0x00RRGGBB)
#define COLOR_BLACK   0x00000000
#define COLOR_WHITE   0x00FFFFFF
#define COLOR_RED     0x00FF0000
#define COLOR_GREEN   0x0000FF00
#define COLOR_BLUE    0x000000FF
#define COLOR_CYAN    0x0000FFFF
#define COLOR_MAGENTA 0x00FF00FF
#define COLOR_YELLOW  0x00FFFF00
#define COLOR_DARK_RED  0x00800000
#define COLOR_DARK_CYAN 0x00008080
#define GRID_COLOR    0x00303050
#define GRID_PURPLE   0x4C08B4

// Constantes del juego
#define PIXEL_SIZE 16  // Tamaño de cada "píxel" en el juego
#define HUD_HEIGHT 32  // Altura para HUD 
#define MAX_GRID_WIDTH 128   
#define MAX_GRID_HEIGHT 96  

#define TICKS_PER_SECOND 18  

// Declaraciones de funciones
void tron_game(void);

#endif
