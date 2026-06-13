#include <shell_io_cmds.h>
#include <lib.h>

void cmd_cat(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char buf[128];
    int64_t n;
    while ((n = read(buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\n';
        write(1, buf, n + 1);
    }
}

void cmd_wc(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char buf[128];
    int64_t n;
    int lines = 0;
    int reads = 0;
    int seen_newline = 0;

    while ((n = read(buf, sizeof(buf) - 1)) > 0) {
        reads++;
        for (int i = 0; i < (int)n; i++) {
            if (buf[i] == '\n') {
                lines++;
                seen_newline = 1;
            }
        }
    }

    if (!seen_newline) {
        lines = reads;
    }

    print("lineas: ");
    printInt(lines);
    println("");
}

static int is_vowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
           c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}

void cmd_filter(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char buf[128];
    char out[128];
    int64_t n;

    while ((n = read(buf, sizeof(buf) - 1)) > 0) {
        int out_len = 0;
        for (int i = 0; i < (int)n; i++) {
            if (!is_vowel(buf[i])) {
                out[out_len++] = buf[i];
            }
        }
        if (out_len > 0) {
            write(1, out, out_len);
            if (out[out_len - 1] != '\n') {
                write(1, "\n", 1);
            }
        } else {
            write(1, "\n", 1);
        }
    }
}
