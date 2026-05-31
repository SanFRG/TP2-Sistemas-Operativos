#include <shell_pipe.h>
#include <lib.h>

/*
 * cat: imprime stdin tal como lo recibe.
 * sys_read desde teclado consume el '\n' del Enter; lo reponemos al escribir
 * para que los comandos del lado derecho del pipe reciban lineas completas.
 */
void cmd_cat(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char buf[128];
    int64_t n;
    while ((n = read(buf, sizeof(buf) - 1)) > 0) {
        write(1, buf, (int)n);
        write(1, "\n", 1);
    }
}

/*
 * wc: cuenta la cantidad de lineas del input.
 */
void cmd_wc(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    char buf[128];
    int64_t n;
    int lines = 0;

    while ((n = read(buf, sizeof(buf) - 1)) > 0) {
        for (int i = 0; i < (int)n; i++) {
            if (buf[i] == '\n') {
                lines++;
            }
        }
    }

    print("lineas: ");
    printInt(lines);
    println("");
}

static int is_vowel(char c) {
    return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
           c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
}

/*
 * filter: filtra las vocales del input (las elimina, pasa el resto).
 */
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
        }
        write(1, "\n", 1);
    }
}
