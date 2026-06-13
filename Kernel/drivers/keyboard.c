#include <keyboard.h>
#include <lib.h>
#include <syscall_dispatcher.h>
#include <exceptions.h>


#define BUFFER_SIZE 64
#define ESC 27
#define ESC_SCANCODE 0x01
#define LEFT_SHIFT_SCANCODE 0x2A
#define RIGHT_SHIFT_SCANCODE 0x36
#define LEFT_CTRL_SCANCODE 0x1D
#define RIGHT_CTRL_SCANCODE 0x1D
#define C_SCANCODE 0x2E
#define D_SCANCODE 0x20
#define RELEASE_MASK 0x80
#define TAB_SIZE 4

extern uint8_t esc_pressed;

static int ctrl_pressed = 0;
static volatile uint8_t ctrl_c_pressed = 0;
static volatile uint8_t ctrl_d_pressed = 0;

static int ascii[] = {
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
static int ascii_shift[] = {
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


static uint8_t scancode_buffer[BUFFER_SIZE];
static int buffer_read = 0;
static int buffer_write = 0;
static int buffer_count = 0;

static int shift_pressed = 0;


void keyboard_handler() {
    uint8_t scancode = getPressedKey();

    if (scancode == ESC_SCANCODE) {
        esc_pressed = 1;
    }


    if (scancode == LEFT_CTRL_SCANCODE || scancode == RIGHT_CTRL_SCANCODE) {
        ctrl_pressed = 1;
    } else if (scancode == (LEFT_CTRL_SCANCODE | RELEASE_MASK) ||
               scancode == (RIGHT_CTRL_SCANCODE | RELEASE_MASK)) {
        ctrl_pressed = 0;
    }


    if (ctrl_pressed && scancode == C_SCANCODE) {
        ctrl_c_pressed = 1;
    }


    if (ctrl_pressed && scancode == D_SCANCODE) {
        ctrl_d_pressed = 1;
    }


    if (buffer_count < BUFFER_SIZE) {
        scancode_buffer[buffer_write] = scancode;
        buffer_write = (buffer_write + 1) % BUFFER_SIZE;
        buffer_count++;
    }

}


char scancode_to_char(uint8_t scancode) {
    static int extended = 0;



    if (scancode == EXTENDED_PREFIX) {
        extended = 1;
        return 0;
    }
    if (extended) {
        extended = 0;
        if (scancode == ARROW_UP_SCANCODE) return KEY_UP;
        if (scancode == ARROW_DOWN_SCANCODE) return KEY_DOWN;
        return 0;
    }

    if (scancode & RELEASE_MASK) {
        uint8_t key = scancode & ~RELEASE_MASK;
        if (key == LEFT_SHIFT_SCANCODE || key == RIGHT_SHIFT_SCANCODE) {
            shift_pressed = 0;
        }
        return 0;
    }

    if (scancode == LEFT_SHIFT_SCANCODE || scancode == RIGHT_SHIFT_SCANCODE) {
        shift_pressed = 1;
        return 0;
    }



    if (ctrl_pressed) {
        return 0;
    }


    uint8_t table_size = sizeof(ascii) / sizeof(ascii[0]);
    if (scancode >= table_size) {
        return 0;
    }


    return shift_pressed ? ascii_shift[scancode] : ascii[scancode];
}

int hasNextKey() {
    return buffer_count > 0;
}

int nextKey() {
    if (!hasNextKey()) {
        return 0;
    }
    return getScancode();
}


uint8_t getScancode() {
    if (buffer_count == 0) {
        return 0;
    }

    uint8_t scancode = scancode_buffer[buffer_read];
    buffer_read = (buffer_read + 1) % BUFFER_SIZE;
    buffer_count--;

    return scancode;
}

uint8_t get_and_clear_ctrl_c(void) {
    uint8_t val = ctrl_c_pressed;
    ctrl_c_pressed = 0;
    return val;
}

uint8_t get_and_clear_ctrl_d(void) {
    uint8_t val = ctrl_d_pressed;
    ctrl_d_pressed = 0;
    return val;
}
