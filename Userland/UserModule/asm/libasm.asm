GLOBAL read
GLOBAL write
GLOBAL get_time
GLOBAL clear_screen
GLOBAL get_ticks
GLOBAL get_key
GLOBAL sleep_ticks
GLOBAL speaker_play
GLOBAL speaker_stop
GLOBAL get_regs
GLOBAL mem_alloc
GLOBAL mem_free
GLOBAL mem_status
GLOBAL getpid
GLOBAL kill_process
GLOBAL block_process
GLOBAL unblock_process
GLOBAL nice_process
GLOBAL waitpid
GLOBAL ps
GLOBAL yield_cpu
GLOBAL create_process
GLOBAL exit_process
GLOBAL check_ctrl_c
GLOBAL loop_inc
GLOBAL sem_open
GLOBAL sem_close
GLOBAL sem_wait
GLOBAL sem_post
GLOBAL pipe_open
GLOBAL create_process_piped_raw
GLOBAL pipe_close
GLOBAL pipe_open_named
GLOBAL set_fd
GLOBAL set_color
GLOBAL _hlt
GLOBAL _cli
GLOBAL _sti

; Syscall numbers (índices en syscall_table)
SYS_READ equ 0
SYS_WRITE equ 1
SYS_CLEAR equ 2
SYS_TIME equ 3
SYS_TICKS equ 4
SYS_GET_KEY equ 5
SYS_SLEEP equ 6
SYS_SPEAKER_PLAY equ 7
SYS_SPEAKER_STOP equ 8
SYS_GET_REGS equ 9
SYS_MEM_ALLOC equ 10
SYS_MEM_FREE equ 11
SYS_MEM_STATUS equ 12
SYS_GETPID equ 13
SYS_KILL equ 14
SYS_BLOCK equ 15
SYS_UNBLOCK equ 16
SYS_NICE equ 17
SYS_WAITPID equ 18
SYS_PS equ 19
SYS_YIELD equ 20
SYS_CREATE_PROCESS equ 21
SYS_EXIT equ 22
SYS_CHECK_CTRL_C equ 23
SYS_LOOP_INC equ 24
SYS_SEM_OPEN equ 25
SYS_SEM_CLOSE equ 26
SYS_SEM_WAIT equ 27
SYS_SEM_POST equ 28
SYS_PIPE_OPEN equ 29
SYS_CREATE_PROCESS_PIPED equ 30
SYS_PIPE_CLOSE equ 31
SYS_SET_COLOR equ 32
SYS_PIPE_OPEN_NAMED equ 33
SYS_SET_FD equ 34
SYS_COUNT equ 35

; Macro para syscall via int 0x80
; %1 = número de syscall
%macro SYSCALL 1
    mov rax, %1
    int 0x80
    ret
%endmacro

; int read(char* buffer, int max_len)
; rdi = buffer, rsi = max_len
read:
    SYSCALL SYS_READ

; int write(int fd, const char* str, int len)
; rdi = fd, rsi = str, rdx = len
write:
    SYSCALL SYS_WRITE

; int get_time(TimeInfo* time_info)
; rdi = time_info pointer --> me parece que no es asi. tiene que ser void
get_time:
    SYSCALL SYS_TIME

; void clear_screen(uint32_t color)
; rdi = color
clear_screen:
    SYSCALL SYS_CLEAR

; uint64_t get_ticks(void)
get_ticks:
    SYSCALL SYS_TICKS

; int get_key(void)
; Returns scancode or 0 if no key
get_key:
    SYSCALL SYS_GET_KEY

; void sleep_ticks(int ticks)
; rdi = ticks
sleep_ticks:
    SYSCALL SYS_SLEEP

; uint64_t speaker_play(uint32_t frequency)
; rdi = frequency (Hz)
speaker_play:
    SYSCALL SYS_SPEAKER_PLAY

; uint64_t speaker_stop(void)
speaker_stop:
    SYSCALL SYS_SPEAKER_STOP

; uint64_t get_regs(RegisterSnapshot* buffer)
; rdi = buffer pointer
get_regs:
    SYSCALL SYS_GET_REGS

; void *mem_alloc(uint64_t size)
; rdi = size
mem_alloc:
    SYSCALL SYS_MEM_ALLOC

; uint64_t mem_free(void *ptr)
; rdi = ptr
mem_free:
    SYSCALL SYS_MEM_FREE

; uint64_t mem_status(MemoryStatus *status)
; rdi = status pointer
mem_status:
    SYSCALL SYS_MEM_STATUS

; int64_t getpid(void)
getpid:
    SYSCALL SYS_GETPID

; int64_t kill_process(uint64_t pid)
; rdi = pid
kill_process:
    SYSCALL SYS_KILL

; int64_t block_process(uint64_t pid)
; rdi = pid
block_process:
    SYSCALL SYS_BLOCK

; int64_t unblock_process(uint64_t pid)
; rdi = pid
unblock_process:
    SYSCALL SYS_UNBLOCK

; int64_t nice_process(uint64_t pid, uint64_t new_priority)
; rdi = pid, rsi = priority
nice_process:
    SYSCALL SYS_NICE

; int64_t waitpid(int64_t pid)
; rdi = pid
waitpid:
    SYSCALL SYS_WAITPID

; int64_t ps(process_info *buffer, uint64_t max_entries)
; rdi = buffer, rsi = max_entries
ps:
    SYSCALL SYS_PS

; int64_t yield_cpu(void)
yield_cpu:
    SYSCALL SYS_YIELD

; int64_t create_process(char *name, void (*entry_point)(void *), void *arg, uint64_t priority, uint64_t foreground)
; rdi = name, rsi = entry_point, rdx = arg, rcx = priority, r8 = foreground
create_process:
    SYSCALL SYS_CREATE_PROCESS

; void exit_process(int exit_code)
; rdi = exit_code  (no retorna: el kernel termina el proceso)
exit_process:
    SYSCALL SYS_EXIT

; int64_t check_ctrl_c(void)
check_ctrl_c:
    SYSCALL SYS_CHECK_CTRL_C

; int64_t loop_inc(void)
loop_inc:
    SYSCALL SYS_LOOP_INC

; int64_t sem_open(const char *name, uint64_t initial_value)
; rdi = name, rsi = initial value
sem_open:
    SYSCALL SYS_SEM_OPEN

; int64_t sem_close(const char *name)
; rdi = name
sem_close:
    SYSCALL SYS_SEM_CLOSE

; int64_t sem_wait(const char *name)
; rdi = name
sem_wait:
    SYSCALL SYS_SEM_WAIT

; int64_t sem_post(const char *name)
; rdi = name
sem_post:
    SYSCALL SYS_SEM_POST

; int64_t pipe_open(void)
pipe_open:
    SYSCALL SYS_PIPE_OPEN

; int64_t create_process_piped_raw(create_proc_piped_args_t *args)
; rdi = pointer to args struct
create_process_piped_raw:
    SYSCALL SYS_CREATE_PROCESS_PIPED

; int64_t pipe_close(int64_t pipe_id)
; rdi = pipe_id
pipe_close:
    SYSCALL SYS_PIPE_CLOSE

; int64_t pipe_open_named(const char *name)
; rdi = name
pipe_open_named:
    SYSCALL SYS_PIPE_OPEN_NAMED

; int64_t set_fd(int64_t fd_index, int64_t fd_value)
; rdi = fd index, rsi = fd value
set_fd:
    SYSCALL SYS_SET_FD

; void set_color(uint8_t attr)
; rdi = attr (byte de atributo VGA)
set_color:
    SYSCALL SYS_SET_COLOR

_hlt:
	sti
	hlt
	ret

_cli:
	cli
	ret


_sti:
	sti
	ret
