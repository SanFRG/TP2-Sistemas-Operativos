GLOBAL outb
GLOBAL inb
GLOBAL setProcessStackASM

section .text

; rdi = port, rsi = value
outb:
	mov dx, di
	mov al, sil
	out dx, al
	ret

; rdi = port; returns byte in rax
inb:
	mov dx, di
	xor rax, rax
	in al, dx
	ret

; rdi = entry, rsi = stack_top, rdx = function, rcx = arg
; returns initial rsp in rax
setProcessStackASM:
	mov r9, rsp
	mov rsp, rsi

	; iretq frame: SS, RSP, RFLAGS, CS, RIP
	push 0x0
	push rsi
	push 0x202
	push 0x08
	push rdi

	; saved registers consumed by popState
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push rdx
	push rcx
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push 0x0
	push 0x0

	mov rax, rsp
	mov rsp, r9
	ret
