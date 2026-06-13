
GLOBAL _cli
GLOBAL _sti
GLOBAL _save_irq
GLOBAL _restore_irq
GLOBAL _atomic_xchg_u8
GLOBAL picMasterMask
GLOBAL picSlaveMask
GLOBAL haltcpu
GLOBAL _hlt

GLOBAL _irq00Handler
GLOBAL _irq01Handler
GLOBAL _irq02Handler
GLOBAL _irq03Handler
GLOBAL _irq04Handler
GLOBAL _irq05Handler
GLOBAL _irq80Handler
GLOBAL user_snapshot
GLOBAL snapshot_ready
GLOBAL esc_pressed

GLOBAL _exception0Handler
GLOBAL _exception6Handler
GLOBAL _defaultIntHandler

EXTERN irqDispatcher
EXTERN exceptionDispatcher
EXTERN syscall_table
EXTERN defaultInterruptDispatcher
EXTERN scheduler_switch
EXTERN process_exit_if_kill_pending

GLOBAL _yield

SECTION .text

%macro pushState 0
	push rax
	push rbx
	push rcx
	push rdx
	push rbp
	push rdi
	push rsi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro

%macro popState 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rsi
	pop rdi
	pop rbp
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro saveUserSnapshot 0

	mov qword [user_snapshot + 0*8], r15
	mov qword [user_snapshot + 1*8], r14
	mov qword [user_snapshot + 2*8], r13
	mov qword [user_snapshot + 3*8], r12
	mov qword [user_snapshot + 4*8], r11
	mov qword [user_snapshot + 5*8], r10
	mov qword [user_snapshot + 6*8], r9
	mov qword [user_snapshot + 7*8], r8
	mov qword [user_snapshot + 8*8], rsi
	mov qword [user_snapshot + 9*8], rdi
	mov qword [user_snapshot + 10*8], rbp
	mov qword [user_snapshot + 11*8], rdx
	mov qword [user_snapshot + 12*8], rcx
	mov qword [user_snapshot + 13*8], rbx
	mov qword [user_snapshot + 14*8], rax

	mov r10, [rsp]
	mov qword [user_snapshot + 15*8], r10
	mov r10, [rsp + 8]
	mov qword [user_snapshot + 16*8], r10
	mov r10, [rsp + 16]
	mov qword [user_snapshot + 17*8], r10
	mov r10, [rsp + 24]
	mov qword [user_snapshot + 18*8], r10
	mov r10, [rsp + 32]
	mov qword [user_snapshot + 19*8], r10

	mov r10, qword [user_snapshot + 5*8]

	mov byte [snapshot_ready], 1
%endmacro

%macro irqHandlerMaster 1
	pushState

	mov byte [esc_pressed], 0

	; rdi = IRQ number
	mov rdi, %1
	call irqDispatcher

	mov al, 20h
	out 20h, al

	popState

	cmp byte [esc_pressed], 1
	jne .skip_snapshot
	saveUserSnapshot

.skip_snapshot:
	iretq
%endmacro

%macro exceptionHandler 1
	pushState

	; rdi = exception number, rsi = saved register frame
	mov rdi, %1
	mov rsi, rsp
	call exceptionDispatcher

	popState
	iretq
%endmacro

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


_save_irq:
	pushfq
	pop rax
	cli
	ret


_restore_irq:
	; rdi = saved RFLAGS
	push rdi
	popfq
	ret


_atomic_xchg_u8:
	; rdi = uint8_t *ptr, rsi = new value; returns old value in rax
	mov al, sil
	xchg byte [rdi], al
	movzx rax, al
	ret

picMasterMask:
	push rbp
    mov rbp, rsp
	; rdi = PIC master mask
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
	; rdi = PIC slave mask
    mov     ax, di
    out	0A1h,al
    pop     rbp
    retn


_irq00Handler:
	pushState

	; rdi = IRQ number
	mov rdi, 0
	call irqDispatcher

	mov al, 20h
	out 20h, al

	; rdi = current saved rsp; scheduler returns next rsp in rax
	mov rdi, rsp
	call scheduler_switch
	mov rsp, rax

	popState
	iretq


_irq01Handler:
	irqHandlerMaster 1


_irq02Handler:
	irqHandlerMaster 2


_irq03Handler:
	irqHandlerMaster 3


_irq04Handler:
	irqHandlerMaster 4


_irq05Handler:
	irqHandlerMaster 5

SYS_COUNT equ 33


_irq80Handler:
	pushState

	; rax = syscall number
	cmp rax, SYS_COUNT
	jae .invalid_syscall

	; syscall_table[rax] returns value in rax
	mov r10, syscall_table
	mov r10, [r10 + rax * 8]

	call r10

	; saved rax slot becomes userland return value
	mov [rsp + 14*8], rax

	call process_exit_if_kill_pending

	popState
	iretq

.invalid_syscall:
	mov qword [rsp + 14*8], -1
	popState
	iretq


_exception0Handler:
	exceptionHandler 0


_exception6Handler:
	exceptionHandler 6


haltcpu:
	cli
	hlt
	ret

_yield:
	int 20h
	ret

SECTION .bss
	aux: resq 1

	user_snapshot: resq 20

	esc_pressed: resb 1
	snapshot_ready: resb 1
