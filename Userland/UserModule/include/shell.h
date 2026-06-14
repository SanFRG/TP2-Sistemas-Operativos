#ifndef SHELL_H
#define SHELL_H

void shell(void);


void cmd_help(int argc, char *argv[]);
void cmd_time(int argc, char *argv[]);
void cmd_mem(int argc, char *argv[]);
void cmd_pid(int argc, char *argv[]);
void cmd_ps(int argc, char *argv[]);
void cmd_memtest(int argc, char *argv[]);
void cmd_testmm(int argc, char *argv[]);
void cmd_registers(int argc, char *argv[]);
void cmd_clear(int argc, char *argv[]);
void cmd_cancion(int argc, char *argv[]);
void cmd_loop(int argc, char *argv[]);
void cmd_kill(int argc, char *argv[]);
void cmd_nice(int argc, char *argv[]);
void cmd_block(int argc, char *argv[]);
void cmd_testproc(int argc, char *argv[]);
void cmd_testprio(int argc, char *argv[]);
void cmd_mvar(int argc, char *argv[]);
void cmd_exit(int argc, char *argv[]);

#endif
