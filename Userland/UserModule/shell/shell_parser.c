#include <shell_internal.h>

int shell_parse_line(char *line, ShellCommandLine *command) {
    char *p;

    if (line == 0 || command == 0) {
        return -1;
    }

    command->argc = 0;
    command->background = 0;
    command->has_pipe = 0;
    command->argc2 = 0;
    for (int i = 0; i < SHELL_MAX_ARGS; i++) {
        command->argv[i] = 0;
        command->argv2[i] = 0;
    }

    p = line;
    while (*p != '\0' && command->argc < SHELL_MAX_ARGS - 1) {
        while (*p == ' ') {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (*p == '&' && (p[1] == '\0' || p[1] == ' ')) {
            command->background = 1;
            break;
        }

        if (*p == '|') {
            command->has_pipe = 1;
            p++;
            /* parse right-hand side of pipe into argv2 */
            while (*p == ' ') p++;
            while (*p != '\0' && command->argc2 < SHELL_MAX_ARGS - 1) {
                while (*p == ' ') p++;
                if (*p == '\0') break;
                command->argv2[command->argc2++] = p;
                while (*p != '\0' && *p != ' ') p++;
                if (*p != '\0') { *p = '\0'; p++; }
            }
            command->argv2[command->argc2] = 0;
            break;
        }

        command->argv[command->argc++] = p;

        while (*p != '\0' && *p != ' ') {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    command->argv[command->argc] = 0;
    return command->argc;
}
