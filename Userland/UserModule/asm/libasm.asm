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
GLOBAL set_color


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
SYS_COUNT equ 33



; Syscall wrappers: args in rdi, rsi, rdx, rcx, r8, r9; return in rax.
%macro SYSCALL 1
    mov rax, %1
    int 0x80
    ret
%endmacro



; rdi = buffer, rsi = max_len
read:
    SYSCALL SYS_READ



; rdi = fd, rsi = buffer, rdx = length, rcx = color
write:
    SYSCALL SYS_WRITE



get_time:
    SYSCALL SYS_TIME



clear_screen:
    SYSCALL SYS_CLEAR


get_ticks:
    SYSCALL SYS_TICKS



get_key:
    SYSCALL SYS_GET_KEY



; rdi = ticks
sleep_ticks:
    SYSCALL SYS_SLEEP



; rdi = frequency
speaker_play:
    SYSCALL SYS_SPEAKER_PLAY


speaker_stop:
    SYSCALL SYS_SPEAKER_STOP



; rdi = RegisterSnapshot *
get_regs:
    SYSCALL SYS_GET_REGS



; rdi = size
mem_alloc:
    SYSCALL SYS_MEM_ALLOC



; rdi = ptr
mem_free:
    SYSCALL SYS_MEM_FREE



; rdi = MemoryStatus *
mem_status:
    SYSCALL SYS_MEM_STATUS


getpid:
    SYSCALL SYS_GETPID



; rdi = pid
kill_process:
    SYSCALL SYS_KILL



; rdi = pid
block_process:
    SYSCALL SYS_BLOCK



; rdi = pid
unblock_process:
    SYSCALL SYS_UNBLOCK



; rdi = pid, rsi = priority
nice_process:
    SYSCALL SYS_NICE



; rdi = pid
waitpid:
    SYSCALL SYS_WAITPID



; rdi = process_info *, rsi = max_entries
ps:
    SYSCALL SYS_PS


yield_cpu:
    SYSCALL SYS_YIELD



; rdi = name, rsi = entry, rdx = arg, rcx = priority, r8 = foreground
create_process:
    SYSCALL SYS_CREATE_PROCESS



; rdi = exit_code
exit_process:
    SYSCALL SYS_EXIT


check_ctrl_c:
    SYSCALL SYS_CHECK_CTRL_C


loop_inc:
    SYSCALL SYS_LOOP_INC



; rdi = name, rsi = initial_value
sem_open:
    SYSCALL SYS_SEM_OPEN



; rdi = name
sem_close:
    SYSCALL SYS_SEM_CLOSE



; rdi = name
sem_wait:
    SYSCALL SYS_SEM_WAIT



; rdi = name
sem_post:
    SYSCALL SYS_SEM_POST


pipe_open:
    SYSCALL SYS_PIPE_OPEN



; rdi = packed args pointer
create_process_piped_raw:
    SYSCALL SYS_CREATE_PROCESS_PIPED



; rdi = pipe_id
pipe_close:
    SYSCALL SYS_PIPE_CLOSE



; rdi = VGA attr
set_color:
    SYSCALL SYS_SET_COLOR
