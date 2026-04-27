#include <tron.h>
#include <tron_ai.h>
#include <lib.h>
#include <sound.h>

static int screen_width = 1024;
static int screen_height = 768;

static int grid_width = 0;
static int grid_height = 0;

// Constantes de dirección
typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

// Estructura del jugador
typedef struct {
    int x;
    int y;
    Direction dir;
    uint32_t color;
    int alive;
} Player;

// Grilla del juego para rastrear posiciones ocupadas

static uint8_t grid[MAX_GRID_HEIGHT][MAX_GRID_WIDTH];

// Puntajes para modo 2 jugadores
static int score_p1 = 0;
static int score_p2 = 0;

// Variables para el contador de FPS
static uint64_t last_frame_ticks = 0;
static int current_fps = 0;
static int last_drawn_fps = -1;  // Para detectar cambios en FPS
static int frame_count = 0;
static uint64_t fps_timer_start = 0;

// Opciones del menú
typedef enum {
    MENU_1_PLAYER,
    MENU_2_PLAYERS,
    MENU_VS_AI,
    MENU_EXIT,
    MENU_COUNT
} MenuOption;

// Mostrar texto centrado
static void draw_centered_text(const char* text, int y, uint32_t color) {
    int len = strlen(text);
    int char_width = 8;
    int x = (screen_width - (len * char_width)) / 2;
    draw_at(text, len, x, y, color);
}

// Función helper para copiar string y retornar nueva posición
static int append_string(char* dest, int idx, const char* src) {
    while (*src) {
        dest[idx++] = *src++;
    }
    return idx;
}

// Dibujar marcador centrado para modo 2 jugadores
static void draw_score(void) {
    // Crear string del marcador: "P1: X  -  P2: Y"
    char score_text[30];
    char num_buffer[12];
    int idx = 0;
    
    idx = append_string(score_text, idx, "P1: ");
    itoa(score_p1, num_buffer);
    idx = append_string(score_text, idx, num_buffer);
    idx = append_string(score_text, idx, "  -  P2: ");
    itoa(score_p2, num_buffer);
    idx = append_string(score_text, idx, num_buffer);
    score_text[idx] = '\0';
    
    // Limpiar y dibujar marcador centrado
    int x = (screen_width - (idx * 8)) / 2;
    draw_rect(x - 10, 8, idx * 8 + 20, 18, COLOR_BLACK);
    draw_at(score_text, idx, x, 12, COLOR_YELLOW);
}

// Dibujar contador de FPS en la esquina superior derecha
static void draw_fps(void) {
    // Solo dibujar si el FPS ha cambiado
    if (current_fps == last_drawn_fps) {
        return;
    }
    
    char fps_text[20];
    char num_buffer[12];
    int idx = 0;
    
    idx = append_string(fps_text, idx, "FPS: ");
    itoa(current_fps, num_buffer);
    idx = append_string(fps_text, idx, num_buffer);
    fps_text[idx] = '\0';
    
    // Limpiar área del FPS (esquina superior derecha)
    int fps_width = idx * 8 + 10;
    draw_rect(screen_width - fps_width - 5, 8, fps_width, 18, COLOR_BLACK);
    
    // Dibujar FPS
    int x = screen_width - fps_width;
    draw_at(fps_text, idx, x, 12, COLOR_GREEN);
    
    // Actualizar último FPS dibujado
    last_drawn_fps = current_fps;
}

// Actualizar el contador de FPS
static void update_fps(void) {
    uint64_t current_ticks = get_ticks();
    
    // Inicializar el timer en el primer frame
    if (fps_timer_start == 0) {
        fps_timer_start = current_ticks;
        frame_count = 0;
        last_frame_ticks = current_ticks;
        return;
    }
    
    frame_count++;
    
    // Calcular FPS en cada frame basado en tiempo transcurrido
    uint64_t elapsed_ticks = current_ticks - fps_timer_start;
    
    if (elapsed_ticks > 0) {
        // FPS = (frames * TICKS_PER_SECOND) / ticks_transcurridos
        current_fps = (frame_count * TICKS_PER_SECOND) / elapsed_ticks;
    }
    
    last_frame_ticks = current_ticks;
}


// Inicializar la grilla del juego
static void init_grid(void) {
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            grid[y][x] = 0;
        }
    }
}

// Verificar si la posición es válida y libre
static int is_position_valid(int x, int y) {
    // Verificar límites
    if (x < 0 || x >= grid_width || y < 0 || y >= grid_height) {
        return 0;
    }
    // Verificar si la posición está ocupada
    return grid[y][x] == 0;
}

// Dibujar un píxel en la grilla
static void draw_grid_unit(int x, int y, uint32_t color) {
    int game_offset_y = HUD_HEIGHT / PIXEL_SIZE;
    draw_rect(x * PIXEL_SIZE, (y + game_offset_y) * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, color);
    grid[y][x] = 1;
}

// Inicializar jugador
static void init_player(Player* p, int x, int y, Direction dir, uint32_t color) {
    p->x = x;
    p->y = y;
    p->dir = dir;
    p->color = color;
    p->alive = 1;
    draw_grid_unit(x, y, color);
}

// Actualizar dirección del jugador según tecla presionada
static void update_player_direction(Player* p, int key, int is_player1) {
    if (is_player1) {
        // Jugador 1: WASD
        if (key == KEY_W && p->dir != DIR_DOWN) {
            p->dir = DIR_UP;
        } else if (key == KEY_S && p->dir != DIR_UP) {
            p->dir = DIR_DOWN;
        } else if (key == KEY_A && p->dir != DIR_RIGHT) {
            p->dir = DIR_LEFT;
        } else if (key == KEY_D && p->dir != DIR_LEFT) {
            p->dir = DIR_RIGHT;
        }
    } else {
        // Jugador 2: IJKL
        if (key == KEY_I && p->dir != DIR_DOWN) {
            p->dir = DIR_UP;
        } else if (key == KEY_K && p->dir != DIR_UP) {
            p->dir = DIR_DOWN;
        } else if (key == KEY_J && p->dir != DIR_RIGHT) {
            p->dir = DIR_LEFT;
        } else if (key == KEY_L && p->dir != DIR_LEFT) {
            p->dir = DIR_RIGHT;
        }
    }
}

// Mover jugador en la dirección actual
static void move_player(Player* p) {
    if (!p->alive) return;
    
    int new_x = p->x;
    int new_y = p->y;
    
    switch (p->dir) {
        case DIR_UP:    new_y--; break;
        case DIR_DOWN:  new_y++; break;
        case DIR_LEFT:  new_x--; break;
        case DIR_RIGHT: new_x++; break;
    }
    
    // Verificar si la nueva posición es válida
    if (!is_position_valid(new_x, new_y)) {
        p->alive = 0;
        return;
    }
    
    // Mover a la nueva posición
    p->x = new_x;
    p->y = new_y;
    draw_grid_unit(new_x, new_y, p->color);
}

// Dibujar bordes visibles del área de juego
static void draw_borders_background(void) {
    int game_offset_y = HUD_HEIGHT;
    for (int x = 0; x < grid_width; x++) {
        draw_rect(x * PIXEL_SIZE, game_offset_y, 1, screen_height - game_offset_y, GRID_COLOR);
        draw_grid_unit(x, 0, GRID_PURPLE);
        draw_grid_unit(x, grid_height - 1, GRID_PURPLE);
    }
    for (int y = 0; y < grid_height; y++) {
        draw_rect(0, game_offset_y + (y * PIXEL_SIZE), screen_width, 1, GRID_COLOR);
        draw_grid_unit(0, y, GRID_PURPLE);
        draw_grid_unit(grid_width - 1, y, GRID_PURPLE);
    }
}

// Esperar a que el jugador presione SPACE (reiniciar) o ENTER (menú)
// Retorna 1 para reiniciar, 0 para volver al menú
static int wait_for_restart_or_menu(void) {
    while (1) {
        int key = get_key();
        if (key != 0 && !(key & 0x80)) {
            if (key == KEY_SPACE) {
                return 1;  // Reiniciar
            } else if (key == KEY_ENTER) {
                return 0;  // Volver al menú
            }
        }
        sleep_ticks(1); 
    }
}

// Bucle de juego genérico
// num_players: 1 o 2, use_ai: 1 si P2 es AI, 0 si es humano o single player
// Retorna 1 si el jugador quiere reiniciar, 0 para volver al menú
static int game_loop(int num_players, int use_ai) {
    clear_screen(COLOR_BLACK);
    init_grid();
    draw_borders_background();
    
    // Reiniciar el contador de FPS
    last_frame_ticks = 0;
    current_fps = 0;
    last_drawn_fps = -1;  // Forzar redibujo inicial
    frame_count = 0;
    fps_timer_start = 0;
    
    // Inicializar jugadores
    Player p1, p2;
    init_player(&p1, grid_width / 5, grid_height / 2, DIR_RIGHT, COLOR_RED);
    
    if (num_players == 2 || use_ai) {
        init_player(&p2, 4 * grid_width / 5, grid_height / 2, DIR_LEFT, COLOR_CYAN);
    }
    
    // Dibujar marcador inicial en modos multijugador
    if (num_players == 2 || use_ai) {
        draw_score();
    }
    
    // Bucle principal
    while (p1.alive && (num_players == 1 || p2.alive)) {

        update_fps();
        draw_fps();

        // Verificar entrada
        int key = get_key();
        if (key != 0 && !(key & 0x80)) {
            update_player_direction(&p1, key, 1);
            if (num_players == 2) {
                update_player_direction(&p2, key, 0);
            }
        }
        
        // Si es modo AI, la AI decide su movimiento
        if (use_ai && p2.alive) {
            GameState state;
            state.grid = (uint8_t*)grid;
            state.grid_width = grid_width;
            state.grid_height = grid_height;
            state.opponent = (AIPlayer*)&p1;
            
            AIPlayer* ai_p2 = (AIPlayer*)&p2;
            ai_p2->dir = ai_decide_move(ai_p2, &state);
        }
        
        // Mover jugador(es)
        move_player(&p1);
        if (num_players == 2 || use_ai) {
            move_player(&p2);
        }
        
        sleep_ticks(1);
    }
    
    // Reproducir sonido cuando el jugador muere
    play_death_sound();
    
    // Game over - Mostrar resultado
    if (num_players == 1 && !use_ai) {
        draw_centered_text("GAME OVER!", screen_height / 2 - 20, COLOR_RED);
    } else {
        // Determinar ganador y actualizar puntajes
        if (!p1.alive && !p2.alive) {
            draw_centered_text("DRAW!", screen_height / 2 - 20, COLOR_YELLOW);
        } else if (p1.alive) {
            score_p1++;
            draw_centered_text("PLAYER 1 WINS!", screen_height / 2 - 20, COLOR_RED);
        } else {
            score_p2++;
            if (use_ai) {
                draw_centered_text("AI WINS!", screen_height / 2 - 20, COLOR_CYAN);
            } else {
                draw_centered_text("PLAYER 2 WINS!", screen_height / 2 - 20, COLOR_CYAN);
            }
        }
    }
    
    draw_centered_text("Press SPACE to restart or ENTER to menu", screen_height / 2 + 20, COLOR_WHITE);
    
    return wait_for_restart_or_menu();
}

static void draw_menu_background(void) {
    int horizon = screen_height / 3;
    int vanishing_y = (horizon > screen_height / 6) ? horizon - screen_height / 6 : 0;
    
    // Líneas horizontales
    for (int i = 0, y = horizon, spacing = 5, grow = 12; i < 50 && y < screen_height; i++, y += spacing) {
        draw_rect(0, y, screen_width, 1, GRID_PURPLE);
        spacing += grow / 6;
        if (i % 5 == 0) grow++;
    }

    // Líneas verticales con perspectiva
    int vanishing_x = screen_width / 2;
    int x_left = -screen_width / 2;
    int total_width = screen_width * 2;
    int den = (screen_height - vanishing_y) ? (screen_height - vanishing_y) : 1;
    
    for (int i = 0; i < 32; i++) {
        int x_bottom = x_left + (total_width * i) / 31;
        for (int yy = horizon; yy < screen_height; yy++) {
            int x = vanishing_x + ((x_bottom - vanishing_x) * (yy - vanishing_y)) / den;
            if (x >= 0 && x < screen_width) {
                draw_rect(x, yy, 1, 1, GRID_PURPLE);
            }
        }
    }
}


// Mostrar el menú principal
MenuOption selected = MENU_1_PLAYER;
static MenuOption show_menu(void) {
    int menu_changed = 1;
    clear_screen(COLOR_BLACK);

    // Dibujar fondo retro
    draw_menu_background();
    
    draw_centered_text("T R O N", 200, COLOR_CYAN);
    // Ayuda de controles
    draw_centered_text("Use W/S to select, ENTER to confirm", 500, COLOR_WHITE);
            
    // Mostrar controles de jugadores
    draw_centered_text("Player 1: WASD   |   Player 2: IJKL", 550, COLOR_CYAN);
            
    while (1) {
        if (menu_changed) {
            // Opciones del menú
            int menu_y = 300;
            
            // 1 Jugador
            if (selected == MENU_1_PLAYER) {
                draw_centered_text("1 PLAYER", menu_y, COLOR_YELLOW);
            } else {
                draw_centered_text("1 PLAYER", menu_y, COLOR_WHITE);
            }
            
            // 2 Jugadores
            menu_y += 40;
            if (selected == MENU_2_PLAYERS) {
                draw_centered_text("2 PLAYERS", menu_y, COLOR_YELLOW);
            } else {
                draw_centered_text("2 PLAYERS", menu_y, COLOR_WHITE);
            }
            
            // VS AI
            menu_y += 40;
            if (selected == MENU_VS_AI) {
                draw_centered_text("VS AI", menu_y, COLOR_YELLOW);
            } else {
                draw_centered_text("VS AI", menu_y, COLOR_WHITE);
            }
            
            // Salir
            menu_y += 40;
            if (selected == MENU_EXIT) {
                draw_centered_text("EXIT", menu_y, COLOR_YELLOW);
            } else {
                draw_centered_text("EXIT", menu_y, COLOR_WHITE);
            }
            
            menu_changed = 0;
        }
        
        // Verificar entrada
        int key = get_key();
        if (key != 0 && !(key & 0x80)) {  
            if (key == KEY_W) {
                // Mover arriba
                if (selected > 0) {
                    selected--;
                    menu_changed = 1;
                }
            } else if (key == KEY_S) {
                // Mover abajo
                if (selected < MENU_COUNT - 1) {
                    selected++;
                    menu_changed = 1;
                }
            } else if (key == KEY_ENTER) {
                // Confirmar selección
                return selected;
            }
        }
        sleep_ticks(1);
    }
}

// Punto de entrada principal del juego Tron
void tron_game(void) {
    // Obtener la resolución de pantalla real
    uint64_t screen_info = get_screen_info();
    screen_width = (int)(screen_info >> 32);
    screen_height = (int)(screen_info & 0xFFFFFFFF);
    
    // Calcular dimensiones de la grilla basadas en la resolución
    grid_width = screen_width / PIXEL_SIZE;
    grid_height = (screen_height - HUD_HEIGHT) / PIXEL_SIZE;
    
    // Guardar escala actual para restaurarla al salir
    int initial_scale = set_scale(-2);  // -2 retorna la escala actual
    
    // Resetear escala a 1 temporalmente para el juego
    for (int i = 0; i < initial_scale - 1; i++) {
        set_scale(-1);  // -1 reduce la escala en 1
    }
    
    while (1) {
        MenuOption option = show_menu();
        
        if (option == MENU_EXIT) {
            selected = MENU_1_PLAYER;
            // Restaurar escala original antes de salir
            for (int i = 0; i < initial_scale - 1; i++) {
                set_scale(1);  // 1 aumenta la escala en 1
            }
            clear_screen(COLOR_BLACK);
            return;
        } else if (option == MENU_1_PLAYER) {
            // Seguir jugando modo 1 jugador hasta que el usuario vuelva al menú
            while (game_loop(1, 0)) {
                // game_loop(1, 0) retorna 1 para reiniciar, 0 para volver al menú
            }
        } else if (option == MENU_2_PLAYERS) {
            // Resetear puntajes al entrar al modo 2 jugadores
            score_p1 = 0;
            score_p2 = 0;
            
            // Seguir jugando modo 2 jugadores hasta que el usuario vuelva al menú
            while (game_loop(2, 0)) {
                // game_loop(2, 0) retorna 1 para reiniciar, 0 para volver al menú
            }
        } else if (option == MENU_VS_AI) {
            score_p1 = 0;
            score_p2 = 0;

            // Jugar contra AI experta
            while (game_loop(2, 1)) {
                // game_loop retorna 1 para reiniciar, 0 para volver al menú
            }
        }
    }
}
