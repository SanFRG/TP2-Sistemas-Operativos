#include <tron_ai.h>

// Direcciones
#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

// Configuración del algoritmo
#define MAX_FLOOD_FILL 1024   // Límite de expansión para flood fill
#define MINIMAX_DEPTH 4       // Profundidad de minimax (3 = AI->OPP->AI)
#define CONTROL_WEIGHT 1000    // Peso para control de territorio
#define SAFETY_WEIGHT 50      // Peso para seguridad
#define PURSUIT_WEIGHT 200    // Peso para persecución activa
#define WIN_SCORE 999999      // Score de victoria
#define LOSE_SCORE -999999    // Score de derrota

// FUNCIONES HELPER

// Verificar si una posición es válida en la grilla
int ai_is_valid_position(int x, int y, GameState* state) {
    if (x < 0 || x >= state->grid_width || y < 0 || y >= state->grid_height) {
        return 0;
    }
    uint8_t (*grid_2d)[MAX_WIDTH] = (uint8_t(*)[MAX_WIDTH])state->grid;
    return grid_2d[y][x] == 0;
}

// Obtener nueva posición según dirección
static void get_next_position(int x, int y, int dir, int* new_x, int* new_y) {
    *new_x = x;
    *new_y = y;
    
    switch (dir) {
        case DIR_UP:    (*new_y)--; break;
        case DIR_DOWN:  (*new_y)++; break;
        case DIR_LEFT:  (*new_x)--; break;
        case DIR_RIGHT: (*new_x)++; break;
    }
}

// Dirección opuesta
static int opposite_direction(int dir) {
    switch (dir) {
        case DIR_UP:    return DIR_DOWN;
        case DIR_DOWN:  return DIR_UP;
        case DIR_LEFT:  return DIR_RIGHT;
        case DIR_RIGHT: return DIR_LEFT;
        default: return dir;
    }
}

// FLOOD FILL - Calcular espacio alcanzable desde una posición
// Retorna el número de celdas alcanzables
static int flood_fill_count(int start_x, int start_y, GameState* state, uint8_t* visited) {
    // Queue simple usando arrays estáticos (evitar recursión)
    static int queue_x[MAX_FLOOD_FILL];
    static int queue_y[MAX_FLOOD_FILL];
    int queue_head = 0;
    int queue_tail = 0;
    int count = 0;
    
    // Marcar inicio
    queue_x[queue_tail] = start_x;
    queue_y[queue_tail] = start_y;
    queue_tail++;
    
    int visited_index = start_y * state->grid_width + start_x;
    visited[visited_index] = 1;
    count = 1;
    
    // BFS iterativo
    while (queue_head < queue_tail && count < MAX_FLOOD_FILL) {
        int x = queue_x[queue_head];
        int y = queue_y[queue_head];
        queue_head++;
        
        // Explorar 4 direcciones
        for (int dir = 0; dir < 4; dir++) {
            int nx, ny;
            get_next_position(x, y, dir, &nx, &ny);
            
            if (ai_is_valid_position(nx, ny, state)) {
                int idx = ny * state->grid_width + nx;
                if (!visited[idx]) {
                    visited[idx] = 1;
                    count++;
                    
                    if (queue_tail < MAX_FLOOD_FILL) {
                        queue_x[queue_tail] = nx;
                        queue_y[queue_tail] = ny;
                        queue_tail++;
                    }
                }
            }
        }
    }
    
    return count;
}

// EVALUACIÓN DE CONTROL DE TERRITORIO (Voronoi-based)
// Calcula cuántas celdas están más cerca de 'ai' que de 'opponent'
static int calculate_territory_control(int ai_x, int ai_y, int opp_x, int opp_y, GameState* state) {
    int ai_territory = 0;
    int neutral = 0;
    
    // Samplear el grid (no revisar TODO para ahorrar tiempo)
    // Revisar cada 2 celdas
    for (int y = 0; y < state->grid_height; y += 2) {
        for (int x = 0; x < state->grid_width; x += 2) {
            if (!ai_is_valid_position(x, y, state)) continue;
            
            // Distancia Manhattan
            int dist_ai = ((x - ai_x) < 0 ? -(x - ai_x) : (x - ai_x)) + 
                          ((y - ai_y) < 0 ? -(y - ai_y) : (y - ai_y));
            int dist_opp = ((x - opp_x) < 0 ? -(x - opp_x) : (x - opp_x)) + 
                           ((y - opp_y) < 0 ? -(y - opp_y) : (y - opp_y));
            
            if (dist_ai < dist_opp) {
                ai_territory++;
            } else if (dist_ai == dist_opp) {
                neutral++;
            }
        }
    }
    
    return ai_territory + neutral / 2;  // Contar la mitad del territorio neutral
}

// EVALUACIÓN DE POSICIÓN - Combina múltiples heurísticas
static int evaluate_position(int ai_x, int ai_y, int opp_x, int opp_y, GameState* state, uint8_t* visited) {
    int score = 0;
    
    // 1. Flood fill - espacio alcanzable desde AI
    for (int i = 0; i < state->grid_width * state->grid_height; i++) {
        visited[i] = 0;
    }
    int ai_space = flood_fill_count(ai_x, ai_y, state, visited);
    
    // 2. Flood fill - espacio alcanzable desde oponente
    for (int i = 0; i < state->grid_width * state->grid_height; i++) {
        visited[i] = 0;
    }
    int opp_space = flood_fill_count(opp_x, opp_y, state, visited);
    
    // 3. Control de territorio (Voronoi)
    int territory = calculate_territory_control(ai_x, ai_y, opp_x, opp_y, state);
    
    // 4. Seguridad - salidas disponibles
    int ai_exits = 0;
    for (int dir = 0; dir < 4; dir++) {
        int nx, ny;
        get_next_position(ai_x, ai_y, dir, &nx, &ny);
        if (ai_is_valid_position(nx, ny, state)) {
            ai_exits++;
        }
    }
    
    int opp_exits = 0;
    for (int dir = 0; dir < 4; dir++) {
        int nx, ny;
        get_next_position(opp_x, opp_y, dir, &nx, &ny);
        if (ai_is_valid_position(nx, ny, state)) {
            opp_exits++;
        }
    }
    
    // 5. AGRESIVIDAD - Distancia al oponente (más cerca = mejor)
    int dx = ai_x - opp_x;
    int dy = ai_y - opp_y;
    int distance = ((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
    
    // Bonus por estar cerca del oponente (distancia óptima: 3-15 celdas)
    int pursuit_bonus = 0;
    if (distance < 20) {
        if (distance < 3) {
            // Demasiado cerca - peligroso
            pursuit_bonus = -50;
        } else if (distance < 8) {
            // Distancia perfecta para presionar
            pursuit_bonus = (8 - distance) * 30;
        } else {
            // Acercarse más
            pursuit_bonus = (20 - distance) * 10;
        }
    }
    
    // 6. CORTAR EL CAMINO - Bonus si bloqueamos posibles salidas del oponente
    int blocking_bonus = 0;
    if (opp_exits < ai_exits) {
        // El oponente tiene menos salidas que nosotros - ventaja táctica
        blocking_bonus = (ai_exits - opp_exits) * 80;
    }
    
    // 7. PRIORIZAR REDUCIR ESPACIO DEL OPONENTE sobre aumentar el propio
    int space_diff = ai_space - opp_space;
    int aggressive_space_bonus = 0;
    if (opp_space < ai_space) {
        // Estamos ganando en espacio - mantener presión
        aggressive_space_bonus = (ai_space - opp_space) * 2;
    }
    
    // Fórmula de evaluación AGRESIVA
    score = space_diff * CONTROL_WEIGHT +
            territory * 2 +
            (ai_exits - opp_exits) * SAFETY_WEIGHT +
            pursuit_bonus * PURSUIT_WEIGHT +
            blocking_bonus +
            aggressive_space_bonus;
    
    return score;
}

// MINIMAX con Alpha-Beta Pruning (sin malloc, iterativo con límite de stack)
// is_maximizing: 1 = turno de AI, 0 = turno de oponente
static int minimax(int ai_x, int ai_y, int opp_x, int opp_y, GameState* state, 
                   int depth, int alpha, int beta, int is_maximizing, 
                   uint8_t* visited, uint8_t* grid_backup) {
    
    // Caso base: profundidad 0 o juego terminado
    if (depth == 0) {
        return evaluate_position(ai_x, ai_y, opp_x, opp_y, state, visited);
    }
    
    // Verificar si hay movimientos disponibles
    int ai_has_moves = 0;
    int opp_has_moves = 0;
    
    for (int dir = 0; dir < 4; dir++) {
        int nx, ny;
        get_next_position(ai_x, ai_y, dir, &nx, &ny);
        if (ai_is_valid_position(nx, ny, state)) {
            ai_has_moves = 1;
            break;
        }
    }
    
    for (int dir = 0; dir < 4; dir++) {
        int nx, ny;
        get_next_position(opp_x, opp_y, dir, &nx, &ny);
        if (ai_is_valid_position(nx, ny, state)) {
            opp_has_moves = 1;
            break;
        }
    }
    
    // Estados terminales
    if (!ai_has_moves && !opp_has_moves) return 0;  // Empate
    if (!ai_has_moves) return LOSE_SCORE + depth;    // AI pierde
    if (!opp_has_moves) return WIN_SCORE - depth;    // AI gana
    
    uint8_t (*grid_2d)[MAX_WIDTH] = (uint8_t(*)[MAX_WIDTH])state->grid;
    
    if (is_maximizing) {
        // Turno del AI - maximizar
        int max_eval = LOSE_SCORE;
        
        for (int dir = 0; dir < 4; dir++) {
            int nx, ny;
            get_next_position(ai_x, ai_y, dir, &nx, &ny);
            
            if (!ai_is_valid_position(nx, ny, state)) continue;
            
            // Simular movimiento: marcar posición actual como ocupada
            uint8_t old_val = grid_2d[ai_y][ai_x];
            grid_2d[ai_y][ai_x] = 1;
            
            // Recursión
            int eval = minimax(nx, ny, opp_x, opp_y, state, depth - 1, alpha, beta, 0, visited, grid_backup);
            
            // Deshacer
            grid_2d[ai_y][ai_x] = old_val;
            
            if (eval > max_eval) {
                max_eval = eval;
            }
            if (eval > alpha) {
                alpha = eval;
            }
            if (beta <= alpha) {
                break;  // Poda alpha-beta
            }
        }
        return max_eval;
        
    } else {
        // Turno del oponente - minimizar
        int min_eval = WIN_SCORE;
        
        for (int dir = 0; dir < 4; dir++) {
            int nx, ny;
            get_next_position(opp_x, opp_y, dir, &nx, &ny);
            
            if (!ai_is_valid_position(nx, ny, state)) continue;
            
            // Simular movimiento
            uint8_t old_val = grid_2d[opp_y][opp_x];
            grid_2d[opp_y][opp_x] = 1;
            
            // Recursión
            int eval = minimax(ai_x, ai_y, nx, ny, state, depth - 1, alpha, beta, 1, visited, grid_backup);
            
            // Deshacer
            grid_2d[opp_y][opp_x] = old_val;
            
            if (eval < min_eval) {
                min_eval = eval;
            }
            if (eval < beta) {
                beta = eval;
            }
            if (beta <= alpha) {
                break;  // Poda alpha-beta
            }
        }
        return min_eval;
    }
}

// AI: Minimax con Alpha-Beta + Evaluación Multi-Criterio AGRESIVA
// Extremadamente difícil de vencer - busca 4 pasos adelante y persigue activamente
static int ai_move(AIPlayer* ai, GameState* state) {
    int current_opposite = opposite_direction(ai->dir);
    
    // Buffer de visited para flood fill (reutilizable)
    static uint8_t visited[MAX_WIDTH * 128];  // 128 = MAX_GRID_HEIGHT
    static uint8_t grid_backup[MAX_WIDTH * 128];  // Backup del grid
    
    int best_dir = ai->dir;
    int best_score = LOSE_SCORE;
    
    uint8_t (*grid_2d)[MAX_WIDTH] = (uint8_t(*)[MAX_WIDTH])state->grid;
    
    // Calcular distancia al jugador para estrategia de persecución
    int dx = state->opponent->x - ai->x;
    int dy = state->opponent->y - ai->y;
    int distance = ((dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy));
    
    // Dirección hacia el oponente (para persecución agresiva)
    int pursuit_dir = -1;
    if ((dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy)) {
        // Más lejos en X
        pursuit_dir = (dx > 0) ? DIR_RIGHT : DIR_LEFT;
    } else {
        // Más lejos en Y
        pursuit_dir = (dy > 0) ? DIR_DOWN : DIR_UP;
    }
    
    // Evaluar cada dirección posible con Minimax
    for (int dir = 0; dir < 4; dir++) {
        // No retroceder
        if (dir == current_opposite) {
            continue;
        }
        
        int nx, ny;
        get_next_position(ai->x, ai->y, dir, &nx, &ny);
        
        // Si no es válido, skip
        if (!ai_is_valid_position(nx, ny, state)) {
            continue;
        }
        
        // Simular movimiento: marcar posición actual como ocupada
        uint8_t old_val = grid_2d[ai->y][ai->x];
        grid_2d[ai->y][ai->x] = 1;
        
        // Llamar minimax (profundidad - 1 porque ya hicimos 1 movimiento)
        // Siguiente turno es del oponente (is_maximizing = 0)
        int score = minimax(nx, ny, state->opponent->x, state->opponent->y, 
                           state, MINIMAX_DEPTH - 1, LOSE_SCORE, WIN_SCORE, 
                           0, visited, grid_backup);
        
        // BONUS AGRESIVO: Si esta dirección nos acerca al oponente
        if (dir == pursuit_dir && distance < 20 && distance > 2) {
            score += PURSUIT_WEIGHT;
        }
        
        // BONUS EXTRA: Si esta dirección bloquea una salida del oponente
        // Verificar si nos movemos a una posición que reduce las opciones del oponente
        int opp_exits_before = 0;
        for (int opp_dir = 0; opp_dir < 4; opp_dir++) {
            int ox, oy;
            get_next_position(state->opponent->x, state->opponent->y, opp_dir, &ox, &oy);
            if (ai_is_valid_position(ox, oy, state)) {
                opp_exits_before++;
            }
        }
        
        // Simular que estamos en la nueva posición
        grid_2d[ny][nx] = 1;
        int opp_exits_after = 0;
        for (int opp_dir = 0; opp_dir < 4; opp_dir++) {
            int ox, oy;
            get_next_position(state->opponent->x, state->opponent->y, opp_dir, &ox, &oy);
            if (ai_is_valid_position(ox, oy, state)) {
                opp_exits_after++;
            }
        }
        grid_2d[ny][nx] = 0;  // Deshacer
        
        // Si reducimos las salidas del oponente, gran bonus
        if (opp_exits_after < opp_exits_before) {
            score += (opp_exits_before - opp_exits_after) * 300;
        }
        
        // Deshacer simulación
        grid_2d[ai->y][ai->x] = old_val;
        
        // Actualizar mejor movimiento
        if (score > best_score) {
            best_score = score;
            best_dir = dir;
        }
    }
    
    // Fallback de emergencia: si no hay buena opción, buscar CUALQUIER válida
    if (best_score <= LOSE_SCORE + 10) {
        for (int dir = 0; dir < 4; dir++) {
            if (dir == current_opposite) continue;
            
            int x, y;
            get_next_position(ai->x, ai->y, dir, &x, &y);
            if (ai_is_valid_position(x, y, state)) {
                return dir;
            }
        }
        return ai->dir;  // Muerte inevitable
    }
    
    return best_dir;
}
int ai_decide_move(AIPlayer* ai, GameState* state) {
    return ai_move(ai, state);
}
