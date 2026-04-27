#ifndef TRON_AI_H
#define TRON_AI_H

#include <stdint.h>

typedef struct {
    int x;
    int y;
    int dir;  
    uint32_t color;
    int alive;
} AIPlayer;

typedef struct {
    uint8_t* grid;       
    int grid_width;
    int grid_height;
    AIPlayer* opponent;  
} GameState;

int ai_decide_move(AIPlayer* ai, GameState* state);

// Función helper: verificar si una posición es válida
int ai_is_valid_position(int x, int y, GameState* state);
#define MAX_WIDTH 128

#endif
