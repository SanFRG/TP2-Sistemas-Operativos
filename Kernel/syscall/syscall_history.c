



#include <syscall_dispatcher.h>
#include <keyboard.h>
#include <lib.h>
#include <interrupts.h>
#include <textConsole.h>
#include <process.h>
#include <pipe.h>


#define HISTORY_SIZE     16
#define HISTORY_LINE_MAX 256

static char history[HISTORY_SIZE][HISTORY_LINE_MAX];
static int history_count = 0;
static int history_head = 0;

static int kstrlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}


static const char *history_get(int logical) {
    int idx = (history_head - history_count + logical + HISTORY_SIZE) % HISTORY_SIZE;
    return history[idx];
}


static void history_add(const char *line) {
    if (line[0] == '\0') return;
    if (history_count > 0) {
        const char *last = history_get(history_count - 1);
        int same = 1;
        for (int k = 0; ; k++) {
            if (last[k] != line[k]) { same = 0; break; }
            if (line[k] == '\0') break;
        }
        if (same) return;
    }
    strncpy(history[history_head], line, HISTORY_LINE_MAX);
    history_head = (history_head + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) history_count++;
}

uint64_t sys_read(uint64_t buffer, uint64_t max_len) {
    char* buf = (char*)buffer;

    if (max_len == 0) return 0;


    int pipe_fd = process_get_current_fd(0);
    if (pipe_fd >= PIPE_FD_MIN) {
        int n = pipe_read(pipe_fd, buf, (int)(max_len > 1 ? max_len - 1 : max_len));
        if (n > 0 && (uint64_t)n < max_len) buf[n] = '\0';
        return (uint64_t)(n < 0 ? 0 : n);
    }

    uint64_t i = 0;


    int nav = history_count;

    while (i + 1 < max_len) {

        if (get_and_clear_ctrl_c()) {
            return (uint64_t)-1;
        }


        if (get_and_clear_ctrl_d()) {
            buf[i] = '\0';
            return i;
        }


        while (!hasNextKey()) {

            if (get_and_clear_ctrl_c()) {
                return (uint64_t)-1;
            }
            if (get_and_clear_ctrl_d()) {
                buf[i] = '\0';
                return i;
            }
            _hlt();
        }

        int scancode = nextKey();

        char c = scancode_to_char(scancode);

        if (c == 0) {
            continue;
        }

        if (c == '\n') {

            tc_write("\n", 1);
            buf[i] = '\0';
            history_add(buf);
            return i;
        } else if (c == KEY_UP || c == KEY_DOWN) {

            if (c == KEY_UP) {
                if (nav > 0) nav--;
                else continue;
            } else {
                if (nav < history_count) nav++;
                else continue;
            }


            while (i > 0) {
                tc_write("\b", 1);
                i--;
            }


            if (nav < history_count) {
                const char *entry = history_get(nav);
                int len = kstrlen(entry);
                if (len > (int)(max_len - 1)) len = (int)(max_len - 1);
                memcpy(buf, entry, len);
                i = (uint64_t)len;
                tc_write(buf, i);
            }

        } else if (c == '\b') {

            if (i > 0) {
                i--;
                tc_write("\b", 1);
            }
        } else if (c == '\t') {


            int col = (int)i;
            int next_stop = ((col / 4) + 1) * 4;
            int spaces = next_stop - col;


            for (int j = 0; j < spaces && i + 1 < max_len; j++) {
                buf[i++] = ' ';
                tc_write(" ", 1);
            }
        } else if (c >= 32 && c < 127) {

            buf[i++] = c;
            tc_write(&c, 1);
        }

    }


    buf[i] = '\0';
    return i;
}
