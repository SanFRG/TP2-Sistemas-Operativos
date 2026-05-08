
GLOBAL _cli
GLOBAL _sti
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
	; Guardar registros en el MISMO ORDEN que RegisterFrame
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

	; Guardar rip, cs, rflags, rsp, ss (pusheados por la CPU en el stack)
	; r10 se usa como scratch; al terminar se restaura desde user_snapshot
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

	; Restaurar r10 al valor original del usuario (antes de usarlo como scratch)
	mov r10, qword [user_snapshot + 5*8]

	mov byte [snapshot_ready], 1
%endmacro

%macro irqHandlerMaster 1
	pushState
	
	mov byte [esc_pressed], 0

	mov rdi, %1      ; ParÃƒÂ¡metro: nÃƒÂºmero de IRQ
	call irqDispatcher

	; signal pic EOI (End of Interrupt)
	mov al, 20h
	out 20h, al

	popState
	
	; DespuÃƒÂ©s de popState, verificar si se presionÃƒÂ³ ESC
	cmp byte [esc_pressed], 1
	jne .skip_snapshot
	saveUserSnapshot

.skip_snapshot:
	iretq
%endmacro

%macro exceptionHandler 1
	pushState

	mov rdi, %1      ; primer parÃƒÂ¡metro: nÃƒÂºmero de excepciÃƒÂ³n
	mov rsi, rsp     ; segundo parÃƒÂ¡metro: puntero a los registros guardados (ExceptionFrame)
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

picMasterMask:
	push rbp
    mov rbp, rsp
    mov ax, di
    out	21h,al
    pop rbp
    retn

picSlaveMask:
	push    rbp
    mov     rbp, rsp
    mov     ax, di 
    out	0A1h,al
    pop     rbp
    retn


;Timer Tick
_irq00Handler:
	irqHandlerMaster 0

;Keyboard
_irq01Handler:
	irqHandlerMaster 1

;Cascade pic never called
_irq02Handler:
	irqHandlerMaster 2

;Serial Port 2 and 4
_irq03Handler:
	irqHandlerMaster 3

;Serial Port 1 and 3
_irq04Handler:
	irqHandlerMaster 4

;USB
_irq05Handler:
	irqHandlerMaster 5

; Syscall (int 0x80) - Llama directamente desde tabla de punteros
_irq80Handler:
	pushState

	; Validar que el numero de syscall sea valido (rax < SYS_COUNT = 13)
	cmp rax, 13
	jae .invalid_syscall

	; Cargar direccion del handler desde la tabla
	mov r10, syscall_table
	mov r10, [r10 + rax * 8]

	call r10

	; El resultado esta en rax, guardarlo en el stack para que popState lo restaure
	mov [rsp + 14*8], rax

	popState
	iretq

.invalid_syscall:
	mov qword [rsp + 14*8], -1   ; rax = -1 indica syscall invalida
	popState
	iretq

;Zero Division Exception
_exception0Handler:
	exceptionHandler 0

;Invalid Opcode Exception
_exception6Handler:
	exceptionHandler 6


haltcpu:
	cli
	hlt
	ret

SECTION .bss
	aux resq 1
	
	; Buffer para el snapshot cuando se presiona ESC
	user_snapshot resq 20
	
	; Flags de control
	esc_pressed resb 1   
	snapshot_ready resb 1  
