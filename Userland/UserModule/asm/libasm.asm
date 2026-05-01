GLOBAL read
GLOBAL write
GLOBAL get_time
GLOBAL clear_screen
GLOBAL set_scale
GLOBAL draw_at
GLOBAL get_ticks
GLOBAL draw_rect
GLOBAL get_key
GLOBAL get_screen_info
GLOBAL sleep_ticks
GLOBAL speaker_play
GLOBAL speaker_stop
GLOBAL get_regs
GLOBAL _hlt
GLOBAL _cli
GLOBAL _sti
GLOBAL trigger_invalid_opcode

; Syscall numbers (índices en syscall_table)
SYS_READ equ 0
SYS_WRITE equ 1
SYS_CLEAR equ 2
SYS_DRAW_AT equ 3
SYS_TIME equ 4
SYS_TICKS equ 5
SYS_SET_SCALE equ 6
SYS_DRAW_RECT equ 7
SYS_GET_KEY equ 8
SYS_GET_SCREEN_INFO equ 9
SYS_SLEEP equ 10
SYS_SPEAKER_PLAY equ 11
SYS_SPEAKER_STOP equ 12
SYS_GET_REGS equ 13
SYS_COUNT equ 14

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

; int set_scale(int delta)
; rdi = delta (1 = aumentar, -1 = disminuir)
set_scale:
    SYSCALL SYS_SET_SCALE

; int draw_at(const char* str, int len, int x, int y, uint32_t color)
; rdi = str, rsi = len, rdx = x, rcx = y, r8 = color
draw_at:
    SYSCALL SYS_DRAW_AT

; uint64_t get_ticks(void)
get_ticks:
    SYSCALL SYS_TICKS

; int draw_rect(int x, int y, int width, int height, uint32_t color)
; rdi = x, rsi = y, rdx = width, rcx = height, r8 = color
draw_rect:
    SYSCALL SYS_DRAW_RECT

; int get_key(void)
; Returns scancode or 0 if no key
get_key:
    SYSCALL SYS_GET_KEY

; uint64_t get_screen_info(void)
; Returns (width << 32) | height
get_screen_info:
    SYSCALL SYS_GET_SCREEN_INFO

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
