#include <textConsole.h>

#define VGA_TEXT_ADDRESS 0xB8000
#define DEFAULT_ATTR 0x07

static volatile uint16_t * const video = (volatile uint16_t *)VGA_TEXT_ADDRESS;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t current_attr = DEFAULT_ATTR;

static uint16_t vga_entry(char c) {
    return ((uint16_t)current_attr << 8) | (uint8_t)c;
}

void tc_set_color(uint8_t attr) {
    current_attr = attr;
}

static void put_entry_at(char c, uint8_t x, uint8_t y) {
    video[y * TEXT_CONSOLE_WIDTH + x] = vga_entry(c);
}

static void scroll(void) {
    for (uint8_t y = 1; y < TEXT_CONSOLE_HEIGHT; y++) {
        for (uint8_t x = 0; x < TEXT_CONSOLE_WIDTH; x++) {
            video[(y - 1) * TEXT_CONSOLE_WIDTH + x] = video[y * TEXT_CONSOLE_WIDTH + x];
        }
    }

    for (uint8_t x = 0; x < TEXT_CONSOLE_WIDTH; x++) {
        put_entry_at(' ', x, TEXT_CONSOLE_HEIGHT - 1);
    }

    if (cursor_y > 0) {
        cursor_y--;
    }
}

static void newline(void) {
    cursor_x = 0;
    cursor_y++;
    if (cursor_y >= TEXT_CONSOLE_HEIGHT) {
        scroll();
    }
}

static void backspace(void) {
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = TEXT_CONSOLE_WIDTH - 1;
    } else {
        return;
    }

    put_entry_at(' ', cursor_x, cursor_y);
}

void tc_clear(void) {
    for (uint8_t y = 0; y < TEXT_CONSOLE_HEIGHT; y++) {
        for (uint8_t x = 0; x < TEXT_CONSOLE_WIDTH; x++) {
            put_entry_at(' ', x, y);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void tc_put_char(char c) {
    if (c == '\n') {
        newline();
        return;
    }

    if (c == '\b') {
        backspace();
        return;
    }

    if (c == '\r') {
        cursor_x = 0;
        return;
    }

    if (c < 32 || c > 126) {
        return;
    }

    put_entry_at(c, cursor_x, cursor_y);
    cursor_x++;

    if (cursor_x >= TEXT_CONSOLE_WIDTH) {
        newline();
    }
}

void tc_write(const char *str, uint64_t length) {
    if (str == 0) {
        return;
    }

    for (uint64_t i = 0; i < length && str[i] != 0; i++) {
        tc_put_char(str[i]);
    }
}

void tc_write_at(const char *str, uint64_t length, uint8_t x, uint8_t y) {
    if (str == 0 || x >= TEXT_CONSOLE_WIDTH || y >= TEXT_CONSOLE_HEIGHT) {
        return;
    }

    for (uint64_t i = 0; i < length && str[i] != 0 && x < TEXT_CONSOLE_WIDTH; i++) {
        if (str[i] >= 32 && str[i] <= 126) {
            put_entry_at(str[i], x++, y);
        }
    }
}

uint16_t tc_get_width(void) {
    return TEXT_CONSOLE_WIDTH;
}

uint16_t tc_get_height(void) {
    return TEXT_CONSOLE_HEIGHT;
}
