GLOBAL outb
GLOBAL inb
GLOBAL setProcessStackASM

section .text

outb:
	mov dx, di     
	mov al, sil     
	out dx, al
	ret

inb:
	mov dx, di    
	xor rax, rax   
	in al, dx
	ret

; rdi: entry point (RIP)
; rsi: stack top
; rdx: valor para RDI del proceso (1er argumento de la funcion)
; rcx: valor para RSI del proceso (2do argumento de la funcion)
setProcessStackASM:
	mov r9, rsp ; guardo rsp actual en r9 ya que es caller-saved
	mov rsp, rsi ; cambio al del proceso nuevo

	; iretq frame: SS, RSP, RFLAGS, CS, RIP
	push 0x0
	push rsi
	push 0x202
	push 0x08
	push rdi

	; GPR frame matching popState order expectations
	push 0x0 ; rax
	push 0x0 ; rbx
	push 0x0 ; rcx
	push 0x0 ; rdx
	push 0x0 ; rbp
	push rdx ; rdi
	push rcx ; rsi
	push 0x0 ; r8
	push 0x0 ; r9
	push 0x0 ; r10
	push 0x0 ; r11
	push 0x0 ; r12
	push 0x0 ; r13
	push 0x0 ; r14
	push 0x0 ; r15

	mov rax, rsp ; devuelvo el rsp del proceso armado
	mov rsp, r9
	ret

