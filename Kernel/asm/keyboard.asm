GLOBAL getPressedKey
section .text

getPressedKey:
	in al, 60h
	ret


