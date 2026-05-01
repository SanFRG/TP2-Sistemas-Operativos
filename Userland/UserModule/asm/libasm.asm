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
GLOBAL _hlt
GLOBAL _cli
GLOBAL _sti
GLOBAL trigger_invalid_opcode

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
SYS_COUNT equ 13

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

; void trigger_invalid_opcode(void)
; Ejecuta la instrucción UD2 para generar excepción #UD (Invalid Opcode)
trigger_invalid_opcode:
    ud2
    ret
