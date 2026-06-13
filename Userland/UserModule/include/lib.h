#ifndef LIB_USER_H
#define LIB_USER_H

#include <stdint.h>

enum {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_CLEAR = 2,
    SYS_TIME = 3,
    SYS_TICKS = 4,
    SYS_GET_KEY = 5,
    SYS_SLEEP = 6,
    SYS_SPEAKER_PLAY = 7,
    SYS_SPEAKER_STOP = 8,
    SYS_GET_REGS = 9,
    SYS_MEM_ALLOC = 10,
    SYS_MEM_FREE = 11,
    SYS_MEM_STATUS = 12,
    SYS_GETPID = 13,
    SYS_KILL = 14,
    SYS_BLOCK = 15,
    SYS_UNBLOCK = 16,
    SYS_NICE = 17,
    SYS_WAITPID = 18,
    SYS_PS = 19,
    SYS_YIELD = 20,
    SYS_CREATE_PROCESS = 21,
    SYS_EXIT = 22,
    SYS_CHECK_CTRL_C = 23,
    SYS_LOOP_INC = 24,
    SYS_SEM_OPEN = 25,
    SYS_SEM_CLOSE = 26,
    SYS_SEM_WAIT = 27,
    SYS_SEM_POST = 28,
    SYS_PIPE_OPEN = 29,
    SYS_CREATE_PROCESS_PIPED = 30,
    SYS_PIPE_CLOSE = 31,
    SYS_SET_COLOR = 32,
    SYS_COUNT = 33
};

typedef struct {
    char *name;
    void (*entry)(void *);
    void *arg;
    int priority;
    int foreground;
    int fd_in;
    int fd_out;
} create_proc_piped_args_t;


typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rsi, rdi, rbp, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} RegisterSnapshot;

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
    uint64_t free_bytes;
    uint64_t successful_allocations;
    uint64_t successful_frees;
    uint64_t failed_allocations;
} MemoryStatus;

typedef struct {
    int pid;
    int parent_pid;
    int priority;
    int foreground;
    int state;
    uint64_t loop_counter;
    uint64_t stack_pointer;
    uint64_t base_pointer;
    char name[32];
} process_info;


extern uint64_t read(char* buffer, int max_len);
extern uint64_t write(int fd, const char* str, int len);
extern uint64_t get_time();
extern uint64_t clear_screen(uint32_t color);
extern void set_color(uint8_t attr);



#define COLOR_DEFAULT 0x07
#define COLOR_RED     0x0C
#define COLOR_GREEN   0x02
#define COLOR_CYAN    0x03
#define COLOR_YELLOW  0x0E
#define COLOR_WHITE   0x0F
extern uint64_t get_ticks(void);
extern int get_key(void);
extern void sleep_ticks(int ticks);
extern uint64_t speaker_play(uint32_t frequency);
extern uint64_t speaker_stop(void);
extern uint64_t get_regs(RegisterSnapshot* buffer);
extern void *mem_alloc(uint64_t size);
extern uint64_t mem_free(void *ptr);
extern uint64_t mem_status(MemoryStatus *status);
extern int64_t getpid(void);
extern int64_t kill_process(uint64_t pid);
extern int64_t block_process(uint64_t pid);
extern int64_t unblock_process(uint64_t pid);
extern int64_t nice_process(uint64_t pid, uint64_t new_priority);
extern int64_t waitpid(int64_t pid);
extern int64_t ps(process_info *buffer, uint64_t max_entries);
extern int64_t yield_cpu(void);
extern int64_t create_process(char *name, void (*entry_point)(void *), void *arg, uint64_t priority, uint64_t foreground);
extern void exit_process(int exit_code);
extern int64_t check_ctrl_c(void);
extern int64_t loop_inc(void);
extern int64_t sem_open(const char *name, uint64_t initial_value);
extern int64_t sem_close(const char *name);
extern int64_t sem_wait(const char *name);
extern int64_t sem_post(const char *name);
extern int64_t pipe_open(void);
extern int64_t pipe_close(int64_t pipe_id);
extern int64_t create_process_piped_raw(create_proc_piped_args_t *args);
int64_t create_process_piped(char *name, void (*entry)(void *), void *arg, int priority, int foreground, int fd_in, int fd_out);


int strlen(const char* str);
int strcasecmp(const char* s1, const char* s2);
int atoi(const char* str);


void print(const char* str);
void println(const char* str);
void print_error(const char* str);
void printInt(int num);
void printHex(uint64_t num);
void print2Digits(int num);


void itoa(int num, char* buffer);
void hexToString(uint64_t num, char* buffer);

#endif
