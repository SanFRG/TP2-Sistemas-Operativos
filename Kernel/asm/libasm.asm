GLOBAL cpuVendor
GLOBAL outb
GLOBAL inb
GLOBAL capture_regs

section .text
	
cpuVendor:
	push rbp
	mov rbp, rsp

	push rbx

	mov rax, 0
	cpuid


	mov [rdi], ebx
	mov [rdi + 4], edx
	mov [rdi + 8], ecx

	mov byte [rdi+13], 0

	mov rax, rdi

	pop rbx

	mov rsp, rbp
	pop rbp
	ret

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

; Captura el estado actual de los registros en el buffer
; rdi = puntero al buffer (debe tener espacio para 20 uint64_t)
capture_regs:
	; Guardar registros en el orden de ExceptionFrame
	mov [rdi + 0*8], r15
	mov [rdi + 1*8], r14
	mov [rdi + 2*8], r13
	mov [rdi + 3*8], r12
	mov [rdi + 4*8], r11
	mov [rdi + 5*8], r10
	mov [rdi + 6*8], r9
	mov [rdi + 7*8], r8
	mov [rdi + 8*8], rsi
	mov [rdi + 9*8], rdi  
	mov [rdi + 10*8], rbp
	mov [rdi + 11*8], rdx
	mov [rdi + 12*8], rcx
	mov [rdi + 13*8], rbx
	mov [rdi + 14*8], rax
	
	; Los siguientes no podemos capturarlos directamente aca
	; porque estamos en medio de una llamada
	; rip, cs, rflags, rsp, ss se capturan aproximadamente
	lea rax, [rel $]       
	mov [rdi + 15*8], rax
	
	mov rax, cs
	mov [rdi + 16*8], rax
	
	pushfq
	pop rax
	mov [rdi + 17*8], rax 
	
	mov rax, rsp
	add rax, 8              
	mov [rdi + 18*8], rax   
	
	mov rax, ss
	mov [rdi + 19*8], rax
	
	ret

